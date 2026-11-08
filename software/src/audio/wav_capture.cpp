#include "wav_capture.hpp"

#include <alsa/asoundlib.h>
#include <sndfile.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <system_error>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

namespace ai_robot::audio {
namespace {

using PcmHandle = std::unique_ptr<snd_pcm_t, decltype(&snd_pcm_close)>;
using WavHandle = std::unique_ptr<SNDFILE, decltype(&sf_close)>;

constexpr int sample_rate = 48'000;
constexpr int channels = 1;
constexpr snd_pcm_uframes_t total_frames = 5 * sample_rate;
constexpr unsigned int pcm_latency_us = 500'000;

static_assert(std::numeric_limits<short>::digits == 15, "Capture requires 16-bit short samples");

CaptureResult failure(const char* stage, const std::string& subject, const std::string& reason)
{
    return {std::string(stage) + " '" + subject + "': " + reason};
}

CaptureResult file_failure(const char* stage, const std::filesystem::path& path, int error)
{
    return failure(stage, path.native(), std::generic_category().message(error));
}

class StagedWav {
public:
    explicit StagedWav(const std::filesystem::path& directory)
        : name_((directory / ".ai-robot-capture-XXXXXX").native()),
          fd_(::mkostemp(name_.data(), O_CLOEXEC))
    {
        if (fd_ < 0) {
            name_.clear();
        }
    }

    ~StagedWav()
    {
        if (fd_ >= 0) {
            ::close(fd_);
        }
        if (!name_.empty()) {
            ::unlink(name_.c_str());
        }
    }

    StagedWav(const StagedWav&) = delete;
    StagedWav& operator=(const StagedWav&) = delete;

    int fd() const noexcept { return fd_; }

    int close() noexcept
    {
        return ::close(std::exchange(fd_, -1));
    }

    int publish(const std::filesystem::path& path) noexcept
    {
        const int status = ::renameat2(AT_FDCWD, name_.c_str(), AT_FDCWD, path.c_str(),
                                       RENAME_NOREPLACE);
        if (status == 0) {
            name_.clear();
        }
        return status;
    }

private:
    std::string name_;
    int fd_;
};

CaptureResult transfer_frames(snd_pcm_t* pcm, SNDFILE* wav, const std::string& device,
                              const std::filesystem::path& path, snd_pcm_uframes_t period_frames)
{
    std::vector<short> samples;
    const auto chunk_frames = std::min(period_frames, total_frames);
    try {
        samples.resize(chunk_frames);
    } catch (const std::bad_alloc&) {
        return failure("allocate PCM chunk", device, "insufficient memory");
    }

    snd_pcm_uframes_t remaining = total_frames;
    while (remaining > 0) {
        const auto requested = std::min(remaining, chunk_frames);
        const auto captured = snd_pcm_readi(pcm, samples.data(), requested);
        if (captured < 0) {
            return failure("read PCM", device, snd_strerror(static_cast<int>(captured)));
        }
        if (captured == 0) {
            return failure("read PCM", device, "read made no progress");
        }

        const auto written = sf_writef_short(wav, samples.data(), captured);
        if (sf_error(wav) != SF_ERR_NO_ERROR) {
            return failure("write WAV", path.native(), sf_strerror(wav));
        }
        if (written != captured) {
            return failure("write WAV", path.native(), "short frame write");
        }
        remaining -= static_cast<snd_pcm_uframes_t>(captured);
    }
    return {};
}

CaptureResult verify_finalized_wav(int fd, const std::filesystem::path& path)
{
    // Some WAV header updates in libsndfile do not propagate errors through sf_close.
    if (::lseek(fd, 0, SEEK_SET) < 0) {
        return file_failure("verify WAV", path, errno);
    }
    SF_INFO info{};
    const WavHandle wav(sf_open_fd(fd, SFM_READ, &info, SF_FALSE), &sf_close);
    if (!wav) {
        return failure("verify WAV", path.native(), sf_strerror(nullptr));
    }
    if ((info.format & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV ||
        (info.format & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16 ||
        (info.format & SF_FORMAT_ENDMASK) == SF_ENDIAN_BIG ||
        info.channels != channels || info.samplerate != sample_rate ||
        info.frames != static_cast<sf_count_t>(total_frames)) {
        return failure("verify WAV", path.native(), "finalized WAV format or frame count is incorrect");
    }
    return {};
}

} // namespace

CaptureResult record_wav_file(const std::filesystem::path& path, const std::string& pcm_device)
{
    if (pcm_device.empty() || pcm_device.find('\0') != std::string::npos) {
        return failure("open PCM", pcm_device, "a valid explicit ALSA PCM name is required");
    }
    if (path.filename().empty() || path.native().find('\0') != std::string::npos) {
        return failure("validate destination", path.native(), "expected a valid file path");
    }
    std::error_code path_error;
    const auto destination = std::filesystem::symlink_status(path, path_error);
    if (path_error && path_error != std::errc::no_such_file_or_directory) {
        return failure("validate destination", path.native(), path_error.message());
    }
    if (std::filesystem::exists(destination)) {
        return failure("validate destination", path.native(), "destination already exists");
    }

    StagedWav staged(path.parent_path());
    if (staged.fd() < 0) {
        return file_failure("create temporary WAV", path, errno);
    }

    SF_INFO info{};
    info.samplerate = sample_rate;
    info.channels = channels;
    info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16 | SF_ENDIAN_LITTLE;
    WavHandle wav(sf_open_fd(staged.fd(), SFM_WRITE, &info, SF_FALSE), &sf_close);
    if (!wav) {
        return failure("open WAV writer", path.native(), sf_strerror(nullptr));
    }

    snd_pcm_t* handle = nullptr;
    int status = snd_pcm_open(&handle, pcm_device.c_str(), SND_PCM_STREAM_CAPTURE, 0);
    if (status < 0) {
        return failure("open PCM", pcm_device, snd_strerror(status));
    }
    PcmHandle pcm(handle, &snd_pcm_close);
    status = snd_pcm_set_params(pcm.get(), SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED,
                                channels, sample_rate, 0, pcm_latency_us);
    if (status < 0) {
        return failure("configure PCM", pcm_device, snd_strerror(status));
    }

    snd_pcm_uframes_t buffer_frames = 0;
    snd_pcm_uframes_t period_frames = 0;
    status = snd_pcm_get_params(pcm.get(), &buffer_frames, &period_frames);
    if (status < 0) {
        return failure("read PCM parameters", pcm_device, snd_strerror(status));
    }
    if (period_frames == 0) {
        return failure("read PCM parameters", pcm_device, "negotiated period is zero");
    }

    if (auto result = transfer_frames(pcm.get(), wav.get(), pcm_device, path, period_frames); !result) {
        return result;
    }
    status = snd_pcm_drop(pcm.get());
    if (status < 0) {
        return failure("stop PCM", pcm_device, snd_strerror(status));
    }
    pcm.reset();

    status = sf_close(wav.release());
    if (status != SF_ERR_NO_ERROR) {
        return failure("finalize WAV", path.native(), sf_error_number(status));
    }
    if (auto result = verify_finalized_wav(staged.fd(), path); !result) {
        return result;
    }
    if (staged.close() < 0) {
        return file_failure("close WAV", path, errno);
    }
    if (staged.publish(path) < 0) {
        return file_failure("publish WAV", path, errno);
    }
    return {};
}

} // namespace ai_robot::audio
