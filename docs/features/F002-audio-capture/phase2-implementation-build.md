# Phase 2 — Implementation & Build Report

**F002 — Audio Capture · APPROVED / PASS · 2026-11-08**

[Feature overview](overview.md) · [Approved Phase 1 design](phase1-requirement-design.md)

## 1. Scope & implementation

`ai_robot_record_wav --device <alsa-pcm> <path.wav>` records 240,000 frames of mono PCM16 at 48 kHz to one little-endian RIFF/WAVE file. The C++17 API owns the recording and reports a stage-specific diagnostic. The CLI returns 0 after publication, 1 for a recording or output error, and 2 for invalid arguments. This implements R1–R3 and R5. R4 is the later listening check with F001. [CLI](../../../software/src/audio/record_wav_main.cpp) · [API](../../../software/src/audio/wav_capture.hpp) · [Implementation](../../../software/src/audio/wav_capture.cpp)

- The capture module opens the caller's ALSA PCM in blocking mode with exact native-endian S16, mono, 48 kHz parameters. It requests a 500 ms buffer time based on the preflight's 24,000-frame buffer at 48 kHz. The read size comes from the negotiated period and the remaining frame budget. A positive partial read advances only by the frames actually written. A negative or zero-progress read, including a reported overrun, aborts capture. [Phase 1 preflight](phase1-requirement-design.md#observed-preflight-evidence) · [ALSA setup and transfer APIs](https://www.alsa-project.org/alsa-doc/alsa-lib/group___p_c_m.html)
- A private temporary sibling receives all frames. After stopping capture, the writer closes and the module reopens the same descriptor to check the finalized WAV metadata. This adds a readback guard to the approved design because libsndfile's WAV header rewrite can lose an error on close. The descriptor closes before `renameat2(RENAME_NOREPLACE)` publishes the file. Scoped cleanup removes the temporary file on handled failure. The interface, frame budget, and publication policy follow the approved design. [libsndfile 1.2.0 WAV close implementation](https://github.com/libsndfile/libsndfile/blob/1.2.0/src/wav.c#L1210-L1233) · [Linux rename semantics](https://man7.org/linux/man-pages/man2/rename.2.html)
- `AI_ROBOT_ENABLE_AUDIO_CAPTURE` defaults to OFF and is Linux-only. Capture and playback share ALSA and libsndfile discovery. Python is discovered only for native audio tests. [CMake](../../../CMakeLists.txt) · [Host test](../../../tests/audio_capture_test.py)

## 2. Build

**PASS · ARM Release.** The existing build script used GNU arm-linux-gnueabihf GCC 11.4.0 and the existing Raspbian Bookworm armhf sysroot. CMake resolved ALSA 1.2.8 and libsndfile 1.2.0. The generated link line points both libraries at the sysroot's `usr/lib/arm-linux-gnueabihf`. The artifact is [build-f002-phase2/armhf/ai_robot_record_wav](../../../build-f002-phase2/armhf/ai_robot_record_wav). [ARM guide](../../project/arm-cross-compilation.md) · [Build and ELF log](../../../build-f002-phase2/armhf-final.log) · [Link command](../../../build-f002-phase2/armhf/CMakeFiles/ai_robot_record_wav.dir/link.txt)

```bash
f002_sysroot="$HOME/.local/share/robot-ai/toolchains/raspbian-bookworm-armhf/sysroot"
bash tools/scripts/build-armhf.sh --sysroot "$f002_sysroot" \
    --build-dir build-f002-phase2/armhf --target ai_robot_record_wav -- \
    -DAI_ROBOT_ENABLE_AUDIO_CAPTURE=ON '-DCMAKE_CXX_FLAGS=-Wall -Wextra -Wpedantic -Werror'
```

## 3. ELF evidence

**PASS · Static ABI check.** `file` reported an ELF 32-bit LSB PIE, ARM, EABI5, dynamically linked. `arm-linux-gnueabihf-readelf -h -A -l -d -n -W` reported:

| Check | Observed |
| --- | --- |
| ELF header | `ELF32`, little-endian, ARM, flags `0x5000400` (Version5 EABI, hard-float ABI). |
| ARM attributes | ARMv7-A, VFPv3-D16, VFP register arguments. |
| Interpreter | `/lib/ld-linux-armhf.so.3`. |
| Dynamic dependencies | `libasound.so.2`, `libsndfile.so.1`, `libstdc++.so.6`, `libgcc_s.so.1`, `libc.so.6`, and `ld-linux-armhf.so.3`. The dynamic table had no RPATH or RUNPATH entries. |
| Artifact identity | Build ID `c8dd77a0fc134c07d42bba0feb99115f2a6cba54`. SHA-256 `163efb05f06240c411fea50f6e515b6475b994e371cdee36fc4c7859a2726313`. |

These ABI details match the Pi's documented Bookworm armhf userspace. [ARM guide](../../project/arm-cross-compilation.md) · [Full inspection](../../../build-f002-phase2/armhf-final.log)

## 4. Tests

Native x86-64 WSL Debug builds used GCC 11.4.0 with `-Wall -Wextra -Wpedantic -Werror`. Audio builds used ALSA 1.2.6.1, libsndfile 1.0.31, and Python 3.10.12.

| Check | Status | Oracle and actual result |
| --- | --- | --- |
| M0 with both options OFF | PASS · 1/1 | Build and run the single smoke test while ALSA, pkg-config, and Python package discovery are disabled. Only M0 built, and `ai_robot_smoke` exited 0 and printed `AI Robot: M0 foundation ready.` [Configure log](../../../build-f002-phase2/host-off.log) · [Final test log](../../../build-f002-phase2/host-off-final.log) |
| F002 with capture ON and playback OFF | PASS · 2/2 | M0 and `audio_capture` passed. An ALSA file PCM with a null slave supplied independent sample values with a distinct tail. Python's WAV reader found 240,000 mono PCM16 frames at 48 kHz and matched all 480,000 bytes to the generated source. [Test source](../../../tests/audio_capture_test.py) · [Final test log](../../../build-f002-phase2/host-capture-final.log) |
| F002 errors and cleanup | PASS · part of `audio_capture` | Missing arguments returned 2. Unknown PCM, missing parent, PCM read error and WAV write error returned 1 with their expected diagnostic stages. The test forced the read error through the ALSA file PCM's failing sink and the write error with a 32 KiB file-size limit. The existing destination retained its original bytes, and the output directory contained only the successful WAV and the sentinel file. [Test source](../../../tests/audio_capture_test.py) · [Final test log](../../../build-f002-phase2/host-capture-final.log) |
| F001 and F002 with both options ON | PASS · 4/4 | M0 and F002 passed with both targets present. The existing F001 tests passed their four supported formats, byte-exact transfer and final padding checks, and all twelve invalid-input assertions. [F001 test source](../../../tests/audio_playback_test.py) · [Final test log](../../../build-f002-phase2/host-both-final.log) |

The first short-source test assumed ALSA would expose raw-file EOF to the caller. The file plugin reports the slave's frame count, so that fixture returned success. The final test forces an actual PCM read error and verifies that the partial recording is discarded. Host coverage exercises the F002 error paths above. Live WM8960 negotiation, discontinuities, and listening remain target checks. [Initial host log](../../../build-f002-phase2/host-capture.log) · [ALSA file plugin read implementation](https://github.com/alsa-project/alsa-lib/blob/v1.2.6.1/src/pcm/pcm_file.c#L604-L624)

Reproduce the combined host run from the repository root.

```bash
cmake -S . -B build-f002-phase2/host-both -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
    -DAI_ROBOT_ENABLE_AUDIO_CAPTURE=ON -DAI_ROBOT_ENABLE_AUDIO_PLAYBACK=ON \
    '-DCMAKE_CXX_FLAGS=-Wall -Wextra -Wpedantic -Werror'
cmake --build build-f002-phase2/host-both --parallel
ctest --test-dir build-f002-phase2/host-both --output-on-failure --no-tests=error --verbose
```

## 5. Deploy

**NOT RUN.** Deployment and Raspberry Pi execution are pending Phase 3 authorization. The local ARM artifact and digest are recorded above.

## 6. Design artifact

The [PlantUML source](phase2-implementation-design.puml) shows the CLI call, staging and PCM setup, frame transfer into libsndfile, WAV readback and publication, and cleanup on errors. Rendering was outside the checks performed.

## 7. Function Inventory

| Function / signature | Source file | Purpose | Calls / dependencies | Errors / return |
| --- | --- | --- | --- | --- |
| CLI · `int main(int argc, char* argv[])` | [record_wav_main.cpp](../../../software/src/audio/record_wav_main.cpp#L6) | Validate the explicit PCM argument and destination. | Calls `record_wav_file` and prints its diagnostic to stderr. | 2 for usage, 1 for a failed capture, 0 after publication. |
| Public Audio API · `CaptureResult record_wav_file(const std::filesystem::path& path, const std::string& pcm_device)` | [wav_capture.hpp](../../../software/src/audio/wav_capture.hpp#L17) · [wav_capture.cpp](../../../software/src/audio/wav_capture.cpp#L146) | Own the destination, PCM session, and WAV lifecycle. | Validates the path, stages the writer, configures ALSA, calls `transfer_frames`, stops PCM, closes and verifies the WAV, and publishes. | Empty diagnostic on success. Stage, subject, and cause on failure. Scoped handles release resources. |
| Public result check · `CaptureResult::operator bool() const noexcept` | [wav_capture.hpp](../../../software/src/audio/wav_capture.hpp#L11) | Let callers test the result. | Checks `diagnostic.empty()`. | True when the recording succeeded. |
| Private · `transfer_frames(pcm, wav, device, path, period_frames)` | [wav_capture.cpp](../../../software/src/audio/wav_capture.cpp#L89) | Move captured frames into the staged WAV using one reusable buffer. | `snd_pcm_readi`, `sf_writef_short`, and the remaining-frame counter. | Success at 240,000 written frames. Diagnostic on allocation failure, read error, zero progress, or incomplete WAV write. |
| Private · `verify_finalized_wav(int fd, const std::filesystem::path& path)` | [wav_capture.cpp](../../../software/src/audio/wav_capture.cpp#L123) | Read back container, encoding, channels, rate, and frame count before publication. | Rewinds the staged descriptor and opens it with libsndfile in read mode. | Diagnostic on seek, open, or metadata mismatch. The read handle closes before returning. |
| Private · `StagedWav(directory)`, `fd()`, `close()`, `publish(path)`, `~StagedWav()` | [wav_capture.cpp](../../../software/src/audio/wav_capture.cpp#L43) | Own the temporary path and descriptor and publish the completed file. | `mkostemp`, `close`, `renameat2`, and `unlink`. | Creation exposes a negative descriptor on failure. Close and publish return system status. Destruction closes a live descriptor and removes an unpublished sibling. |
| Private · `failure(stage, subject, reason)`, `file_failure(stage, path, error)` | [wav_capture.cpp](../../../software/src/audio/wav_capture.cpp#L33) | Format the caller-visible diagnostic. | Uses the given reason or the message for the supplied OS error. | Returns a failed `CaptureResult`. |

## 8. Issues & handoff

**APPROVED / PASS.** The Engineer approved the implementation, build results, and documentation on 2026-11-08. Raspberry Pi execution and listening verification through F001 belong to Phase 3. The Phase 1 tool-based preflight remains separate evidence. [Phase 1 preflight](phase1-requirement-design.md#observed-preflight-evidence)

The Engineer authorized Phase 3 target execution on 2026-11-08.
