# Phase 2 — Implementation & Build Report

[F001 — Audio Playback overview](overview.md) · [Approved Phase 1 design](phase1-requirement-design.md)

## 1. Scope & implementation

F001 implements R1–R5 using decisions D1–D3. Verification follows D4 below. The CLI accepts an explicit `--device` and a local file, with exit codes 0 for completed transfer and drain, 1 for playback errors, and 2 for usage errors. The playback module validates libsndfile metadata before opening ALSA, then streams the decoded samples through a reusable buffer. It handles partial writes, drains on success, and drops queued frames on transfer or drain failure. Scoped handles release both resources. Validation accepts standard and extensible little-endian RIFF/WAVE headers when the decoded encoding and channel/rate combination meet D3. ALSA receives the source rate and channel count with a 200 ms latency request.

- [CLI](../../../software/src/audio/play_wav_main.cpp) and [playback API](../../../software/src/audio/wav_playback.hpp) / [implementation](../../../software/src/audio/wav_playback.cpp)
- [CMake](../../../CMakeLists.txt) adds `ai_robot_play_wav` behind the `AI_ROBOT_ENABLE_AUDIO_PLAYBACK` option, which defaults to OFF. Python 3 is required only for the opt-in native tests.
- [Host tests](../../../tests/audio_playback_test.py) generate disposable fixtures and exercise the CLI through CTest.

### Playback ending update from Phase 3

After configuring the PCM, the current player queries the negotiated period and allocates one reusable interleaved buffer of that size. Each read decodes up to one period. The existing partial-write loop submits the whole chunk, with silence filling only the unused frames in a short final read. A period-aligned file needs no padding. The player drains after the last chunk is submitted.

**Latest build and tests (Phase 3, 2026-11-08):** The Release ARM build passed with GCC 11.4.0 against the Bookworm armhf sysroot. `build-armhf-f001-chunk/ai_robot_play_wav` was verified as little-endian ARMv7 ELF32, EABI5 hard-float, with loader `/lib/ld-linux-armhf.so.3` and build ID `cea0cf84c3e7c599625cb98891f86b5c4417b53a`. Host CTest passed **3/3**, including M0, the format and error cases, byte-exact transfer of 4,800 aligned frames without padding, and byte-exact transfer of 4,801 frames with 2,399 silent frames. The Engineer confirmed clean playback of the original WAV through WM8960. [Phase 3 build and target evidence](phase3-execution-debug.md#negotiated-period-chunk-experiment)

Sections 2–5 retain the approved Phase 2 baseline evidence.

## 2. Build

**VERIFIED · PASS.** ARM Release build in WSL with GCC/G++ 11.4.0 and the Bookworm armhf sysroot from the [project guide](../../project/arm-cross-compilation.md). The existing build script produced `build-armhf/ai_robot_play_wav`.

```bash
sysroot="$HOME/.local/share/robot-ai/toolchains/raspbian-bookworm-armhf/sysroot"
bash tools/scripts/build-armhf.sh --sysroot "$sysroot" --target ai_robot_play_wav -- \
    -DAI_ROBOT_ENABLE_AUDIO_PLAYBACK=ON
```

CMake resolved ALSA 1.2.8 and libsndfile 1.2.0. The cache and link command selected both libraries under the supplied sysroot's `usr/lib/arm-linux-gnueabihf`.

## 3. ELF evidence

**VERIFIED · PASS.** `file build-armhf/ai_robot_play_wav` reported an ELF 32-bit LSB ARM PIE executable, EABI5, dynamically linked. `arm-linux-gnueabihf-readelf -h -A -l` confirmed ARMv7, VFPv3-D16, VFP register arguments, EABI5 hard-float, and interpreter `/lib/ld-linux-armhf.so.3`. The ABI matches the verified Pi userspace.

`arm-linux-gnueabihf-readelf -d` listed `libasound.so.2`, `libsndfile.so.1`, `libstdc++.so.6`, `libc.so.6`, and `ld-linux-armhf.so.3` with no RPATH or RUNPATH.

## 4. Tests

| WSL x86-64 test | Status | Observed evidence |
| --- | --- | --- |
| `audio_playback_valid` | PASS | All four 16-bit channel/rate combinations returned 0 through ALSA `null`. A file PCM backed by `null` recorded byte-exact 48 kHz stereo from a PCM16 extensible WAV with a different file suffix. The 2,053-frame fixture crossed two full blocks and a partial block. |
| `audio_playback_invalid` | PASS | Twelve cases verified usage, missing, unreadable or nonregular paths, invalid header, empty data, unsupported encoding, rate or channel count, and an unknown PCM. Pairing invalid WAVs with an unknown PCM verified that metadata validation runs before device open. |
| `ai_robot_smoke` | PASS | Native M0 CTest passed with playback enabled. The existing `tools/scripts/test.sh` also passed 1/1 with playback OFF before the audio development packages were installed. The M0 executable printed its original startup message. |

The final host CTest run passed 3/3 with GCC 11.4.0, ALSA 1.2.6.1, and libsndfile 1.0.31 on Ubuntu 22.04 WSL. The first run exposed an overly specific empty-file diagnostic assertion. The assertion was corrected to check the stage and reason separately.

```bash
cmake -S . -B build-audio-host -DCMAKE_BUILD_TYPE=Debug -DAI_ROBOT_ENABLE_AUDIO_PLAYBACK=ON \
    -DBUILD_TESTING=ON '-DCMAKE_CXX_FLAGS=-Wall -Wextra -Wpedantic'
cmake --build build-audio-host --parallel
ctest --test-dir build-audio-host --output-on-failure --no-tests=error --verbose
```

## 5. Deploy

**NOT RUN during Phase 2.** Deployment and target playback were recorded separately in the [Phase 3 report](phase3-execution-debug.md).

## 6. Design artifact

The [PlantUML source](phase2-implementation-design.puml) shows CLI and WAV validation, the negotiated period as the chunk size, silence padding within the final incomplete chunk, drain, and cleanup on success or error. The source was reviewed against the implementation. Rendering was not checked.

## 7. Function Inventory

This inventory reflects the current source, including the [approved final-chunk change](phase3-execution-debug.md#negotiated-period-chunk-experiment). Build and test results above describe the Phase 2 baseline.

| Function / signature | Source file | Purpose | Calls / dependencies | Errors / return |
| --- | --- | --- | --- | --- |
| CLI entry · `int main(int argc, char* argv[])` | [play_wav_main.cpp](../../../software/src/audio/play_wav_main.cpp#L6) | Parse the required `--device` argument and WAV path and report the outcome to stderr when needed. | `play_wav_file`, `PlaybackResult::operator bool` | Returns 2 for usage errors, 1 with a diagnostic for playback errors, or 0 after successful transfer and drain. |
| Public API · `PlaybackResult play_wav_file(const std::filesystem::path& path, const std::string& pcm_device)` | [wav_playback.hpp](../../../software/src/audio/wav_playback.hpp#L17), [wav_playback.cpp](../../../software/src/audio/wav_playback.cpp#L112) | Check the input, open and validate WAV metadata, configure the selected blocking PCM, read its negotiated period, transfer frames, and drain. | `is_regular_file`, libsndfile open, `validate_wav_format`, ALSA open/configure, `snd_pcm_get_params`, `transfer_frames`, ALSA drain and drop, scoped handles | Returns an empty diagnostic on success or a stage-specific diagnostic on failure. Attempts PCM drop on transfer or drain error. Scoped handles release any acquired PCM and WAV resources on return. |
| Public result check · `explicit operator bool() const noexcept` | [wav_playback.hpp](../../../software/src/audio/wav_playback.hpp#L11) | Let the CLI and module test whether a `PlaybackResult` succeeded. | `diagnostic.empty()` | Returns true when the diagnostic is empty. Does not throw. |
| Private · `std::string validate_wav_format(const SF_INFO& info)` | [wav_playback.cpp](../../../software/src/audio/wav_playback.cpp#L30) | Enforce the RIFF/WAVE container, signed PCM16, channel/rate limits, and positive frame count before opening ALSA. | libsndfile format flags and metadata | Returns an empty string for valid metadata or a reason for rejection. |
| Private · `PlaybackResult transfer_frames(SNDFILE* wav, const SF_INFO& info, const std::string& path, snd_pcm_t* pcm, const std::string& device, snd_pcm_uframes_t period_frames)` | [wav_playback.cpp](../../../software/src/audio/wav_playback.cpp#L68) | Decode the WAV into a one-period buffer and zero-fill only the unused samples in the final incomplete chunk before writing it. | `sf_readf_short`, `sf_error`, `write_frames`, `failure` | Returns success when all chunks are submitted, or a diagnostic for an unsupported period size, allocation failure, or read/write error. The caller owns cleanup and drain. |
| Private · `PlaybackResult write_frames(snd_pcm_t* pcm, const std::string& device, const short* next, snd_pcm_uframes_t pending, int channels)` | [wav_playback.cpp](../../../software/src/audio/wav_playback.cpp#L50) | Write interleaved frames and advance the sample pointer after each partial write. | `snd_pcm_writei`, `failure` | Returns success when the block is sent, or a diagnostic on a negative write or zero progress. |
| Private · `PlaybackResult failure(const char* stage, const std::string& subject, const std::string& reason)` | [wav_playback.cpp](../../../software/src/audio/wav_playback.cpp#L25) | Format a stage, file or PCM name, and cause for the CLI diagnostic. | String construction | Returns a failed result with a nonempty diagnostic. |

The implementation's `WavHandle` and `PcmHandle` aliases use scoped deleters `sf_close` and `snd_pcm_close`. They run on every return after acquisition, including validation or configuration errors.

## 8. Issues & handoff

**APPROVED.** The Engineer approved Phase 2 before target execution. The host sinks verify software behavior and sample transfer. Short writes and live PCM configure, write, and drain failures were reviewed in code but were not forced in tests. The [Phase 1 `aplay` baseline](phase1-requirement-design.md#hardware-preflight) and the [Task 3 M0 target run](../../project/arm-cross-compilation.md#target-smoke-test) remain separate from F001 application verification. The original WAV passed with the approved period-chunk player. The full WM8960 format matrix remains for Phase 4.

The [Phase 3 report](phase3-execution-debug.md) records the separately authorized target smoke test.
