#include "wav_playback.hpp"

#include <alsa/asoundlib.h>
#include <sndfile.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <system_error>

namespace ai_robot::audio {
namespace {

using WavHandle = std::unique_ptr<SNDFILE, decltype(&sf_close)>;
using PcmHandle = std::unique_ptr<snd_pcm_t, decltype(&snd_pcm_close)>;

constexpr sf_count_t frames_per_block = 1024;
constexpr int max_channels = 2;
constexpr unsigned int pcm_latency_us = 200'000;

static_assert(std::numeric_limits<short>::digits == 15, "Playback requires 16-bit short samples");

PlaybackResult failure(const char* stage, const std::string& subject, const std::string& reason)
{
    return {std::string(stage) + " '" + subject + "': " + reason};
}

std::string validate_wav_format(const SF_INFO& info)
{
    const int container = info.format & SF_FORMAT_TYPEMASK;
    if ((container != SF_FORMAT_WAV && container != SF_FORMAT_WAVEX) ||
        (info.format & SF_FORMAT_ENDMASK) == SF_ENDIAN_BIG ||
        (info.format & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
        return "expected RIFF/WAVE with signed 16-bit PCM";
    }
    if (info.channels != 1 && info.channels != max_channels) {
        return "unsupported channel count " + std::to_string(info.channels) + " (expected 1 or 2)";
    }
    if (info.samplerate != 44'100 && info.samplerate != 48'000) {
        return "unsupported sample rate " + std::to_string(info.samplerate) + " Hz (expected 44100 or 48000)";
    }
    if (info.frames <= 0) {
        return "WAV contains no audio frames";
    }
    return {};
}

PlaybackResult write_frames(snd_pcm_t* pcm, const std::string& device, const short* next,
                            snd_pcm_uframes_t pending, int channels)
{
    while (pending > 0) {
        const snd_pcm_sframes_t written = snd_pcm_writei(pcm, next, pending);
        if (written < 0) {
            return failure("write PCM", device, snd_strerror(static_cast<int>(written)));
        }
        if (written == 0) {
            return failure("write PCM", device, "write made no progress");
        }
        // ALSA counts frames, but the interleaved buffer advances in samples.
        next += written * channels;
        pending -= static_cast<snd_pcm_uframes_t>(written);
    }
    return {};
}

PlaybackResult transfer_frames(SNDFILE* wav, const SF_INFO& info, const std::string& path,
                               snd_pcm_t* pcm, const std::string& device,
                               snd_pcm_uframes_t period_frames)
{
    std::array<short, frames_per_block * max_channels> samples;
    sf_count_t unread = info.frames;
    while (unread > 0) {
        const sf_count_t requested = std::min(unread, frames_per_block);
        const sf_count_t decoded = sf_readf_short(wav, samples.data(), requested);
        if (sf_error(wav) != SF_ERR_NO_ERROR) {
            return failure("read WAV", path, sf_strerror(wav));
        }
        if (decoded != requested) {
            return failure("read WAV", path, "unexpected end of audio data");
        }

        auto result = write_frames(pcm, device, samples.data(),
                                   static_cast<snd_pcm_uframes_t>(decoded), info.channels);
        if (!result) {
            return result;
        }
        unread -= decoded;
    }

    // Finish on a period boundary with at least one silent period before drain.
    const auto remainder = static_cast<std::uintmax_t>(info.frames) % period_frames;
    auto silence_frames = period_frames + (period_frames - remainder) % period_frames;
    samples.fill(0);
    while (silence_frames > 0) {
        const auto block = std::min<std::uintmax_t>(silence_frames, frames_per_block);
        auto result = write_frames(pcm, device, samples.data(),
                                   static_cast<snd_pcm_uframes_t>(block), info.channels);
        if (!result) {
            return result;
        }
        silence_frames -= block;
    }
    return {};
}

} // namespace

PlaybackResult play_wav_file(const std::filesystem::path& path, const std::string& pcm_device)
{
    if (pcm_device.empty()) {
        return failure("open PCM", pcm_device, "an explicit ALSA PCM name is required");
    }

    std::error_code file_error;
    if (!std::filesystem::is_regular_file(path, file_error)) {
        return failure("open WAV", path.native(),
                       file_error ? file_error.message() : "expected a regular file");
    }

    SF_INFO info{};
    const WavHandle wav(sf_open(path.c_str(), SFM_READ, &info), &sf_close);
    if (!wav) {
        return failure("open WAV", path.native(), sf_strerror(nullptr));
    }
    if (const auto reason = validate_wav_format(info); !reason.empty()) {
        return failure("validate WAV", path.native(), reason);
    }

    snd_pcm_t* handle = nullptr;
    int status = snd_pcm_open(&handle, pcm_device.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (status < 0) {
        return failure("open PCM", pcm_device, snd_strerror(status));
    }
    const PcmHandle pcm(handle, &snd_pcm_close);
    status = snd_pcm_set_params(pcm.get(), SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED,
                                static_cast<unsigned int>(info.channels),
                                static_cast<unsigned int>(info.samplerate), 1, pcm_latency_us);
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

    auto result = transfer_frames(wav.get(), info, path.native(), pcm.get(), pcm_device, period_frames);
    if (!result) {
        snd_pcm_drop(pcm.get());
        return result;
    }

    status = snd_pcm_drain(pcm.get());
    if (status < 0) {
        snd_pcm_drop(pcm.get());
        return failure("drain PCM", pcm_device, snd_strerror(status));
    }
    return {};
}

} // namespace ai_robot::audio
