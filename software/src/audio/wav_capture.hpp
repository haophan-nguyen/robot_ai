#pragma once

#include <filesystem>
#include <string>

namespace ai_robot::audio {

struct CaptureResult {
    std::string diagnostic;

    explicit operator bool() const noexcept
    {
        return diagnostic.empty();
    }
};

CaptureResult record_wav_file(const std::filesystem::path& path, const std::string& pcm_device);

} // namespace ai_robot::audio
