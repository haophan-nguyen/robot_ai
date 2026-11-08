#include "wav_capture.hpp"

#include <cstdio>
#include <iostream>
#include <string_view>

#include <signal.h>

int main(int argc, char* argv[])
{
    if (argc != 4 || std::string_view(argv[1]) != "--device" || !argv[2][0] || !argv[3][0]) {
        std::cerr << "Usage: ai_robot_record_wav --device <alsa-pcm> <path.wav>\n";
        return 2;
    }

    // Let the WAV writer report file-size limit errors and clean up its temporary output.
    struct sigaction action{};
    action.sa_handler = SIG_IGN;
    if (::sigemptyset(&action.sa_mask) == -1 || ::sigaction(SIGXFSZ, &action, nullptr) == -1) {
        std::perror("configure SIGXFSZ");
        return 1;
    }

    const auto result = ai_robot::audio::record_wav_file(argv[3], argv[2]);
    if (!result) {
        std::cerr << result.diagnostic << '\n';
        return 1;
    }
    return 0;
}
