# F002 — Audio Capture Technical Handover

F002 gives the Voice Command Robot a microphone input. The C++17 recorder opens an explicit ALSA capture PCM and records once in the foreground. It publishes a new little-endian RIFF/WAVE file containing exactly 240,000 mono signed 16-bit PCM frames at 48 kHz. That is 5.000 seconds of audio and 480,000 sample bytes. Wall time includes device setup and file finalization. The MVP uses the Pi's WM8960 microphone route. F001 replays the finished file through the WM8960 speaker for acceptance. Continuous streaming, VAD, speech recognition, simultaneous capture and playback, and echo cancellation are outside the MVP. [Approved requirements](phase1-requirement-design.md#1-feature-understanding) · [Subsystem context](../../sessions/2026/2026-11-03.md)

## Code map and contracts

| Entry point | Responsibility |
| --- | --- |
| [Recorder CLI](../../../software/src/audio/record_wav_main.cpp#L9) | `ai_robot_record_wav --device <alsa-pcm> <path.wav>` requires both values. Exit 2 prints usage for invalid arguments. Exit 1 prints a stage-specific diagnostic for signal setup, capture or output failure. Exit 0 follows successful publication and produces no application output. |
| [Capture API](../../../software/src/audio/wav_capture.hpp#L8) · [Implementation](../../../software/src/audio/wav_capture.cpp#L146) | `CaptureResult record_wav_file(const std::filesystem::path& path, const std::string& pcm_device)` is a blocking call in `ai_robot::audio`. It owns the capture session and the staged file. `CaptureResult::operator bool()` tests whether `diagnostic` is empty. The caller selects the PCM and supplies a fresh destination under an existing writable directory. |
| [transfer_frames](../../../software/src/audio/wav_capture.cpp#L89) | Takes open PCM and WAV writer handles, diagnostic subjects and the negotiated period in frames. Copies captured frames into the WAV, returning an empty diagnostic only after the full frame budget is written. |
| [verify_finalized_wav](../../../software/src/audio/wav_capture.cpp#L123) · [StagedWav](../../../software/src/audio/wav_capture.cpp#L43) | `verify_finalized_wav(int fd, const std::filesystem::path& path)` checks finalized metadata from the open descriptor. `StagedWav` owns the descriptor and temporary name, publishes with no replacement, and removes unpublished output on scope exit. |
| [CMake](../../../CMakeLists.txt#L18) · [Capture test](../../../tests/audio_capture_test.py) | `AI_ROBOT_ENABLE_AUDIO_CAPTURE` defaults to OFF and is Linux-only. Capture and playback share ALSA and libsndfile discovery when either option is ON. Native builds with tests enabled register `audio_capture` when capture is ON. With both options OFF, the M0 build needs neither audio library. |

## From microphone to published WAV

1. **Validate and stage.** After checking usage, `main` ignores `SIGXFSZ` for the recorder process so an output-size limit can be reported as a write failure. The API validates the device name and path and rejects an existing destination entry, including a symlink. `mkostemp` creates a private `.ai-robot-capture-*` sibling with a close-on-exec descriptor. libsndfile opens it in write mode for little-endian WAV PCM16. `SF_FALSE` tells libsndfile to leave descriptor ownership with `StagedWav`. Staging in the destination directory permits one atomic publication step later.
2. **Configure and read.** ALSA opens only the supplied PCM in blocking capture mode. The app requests interleaved native-endian S16, one channel and exactly 48 kHz, with software resampling disabled in `snd_pcm_set_params`. The 500 ms latency request came from the preflight's 24,000-frame buffer at 48 kHz. The app queries the actual buffer and period sizes and uses the period, capped at the total frame budget, to size one reusable buffer. A failed query or zero period aborts. The verified `plughw` route delivered app-facing mono by averaging the two WM8960 hardware channels at 48 kHz.
3. **Account for actual frames.** One mono frame is one native `short` sample. `snd_pcm_readi` requests the smaller of the chunk size and the remaining budget. A positive partial read writes only the returned frames via `sf_writef_short`, which serializes them as little-endian WAV samples. The remaining budget drops by that count after a complete write. A negative read, including a reported overrun, zero progress, a writer error or a short write aborts. Every frame in the file comes from capture.
4. **Stop, finalize and check.** Once the budget is exhausted, `snd_pcm_drop` stops capture and the scoped PCM handle closes. The code checks `sf_close` on the WAV writer, rewinds the same staged descriptor, reopens it in read mode and verifies the container, PCM16 subtype, endianness, channels, rate and frame count. The readback guard catches header problems that the writer's close result can miss. It checks metadata. The independent acceptance check also reads the payload.
5. **Publish or clean up.** Close the staged descriptor, then publish with `renameat2(RENAME_NOREPLACE)`. The early existence check rejects an occupied path promptly, while the atomic rename preserves a destination created during recording. The output directory must support this Linux operation. On handled failures, scoped handles release the PCM and libsndfile resources and the staging owner attempts to remove the temporary sibling. Abrupt process termination can leave the sibling behind.

[Capture implementation](../../../software/src/audio/wav_capture.cpp) · [Sequence diagram](phase2-implementation-design.puml) · [WM8960 preflight](phase1-requirement-design.md#observed-preflight-evidence)

## Build and run

The commands below are reproduction steps based on the inspected scripts and historical runs. Run the host commands in Bash from the repository root on Linux or WSL. The native build needs a C++17 compiler, CMake 3.16+, a build tool, `pkg-config`, `libasound2-dev` and `libsndfile1-dev` on Ubuntu/Debian, plus Python 3 for native audio tests. The saved host runs used Ubuntu 22.04 WSL and GCC 11.4.0. [Phase 2 build and test results](phase2-implementation-build.md#4-tests) · [CMake](../../../CMakeLists.txt)

### Linux host

```bash
cmake -S . -B build-f002-handover-host -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  -DAI_ROBOT_ENABLE_AUDIO_CAPTURE=ON -DAI_ROBOT_ENABLE_AUDIO_PLAYBACK=ON
cmake --build build-f002-handover-host --parallel
(cd build-f002-handover-host && ctest --output-on-failure -V)
```

Expect four tests with both options ON, two with capture alone, and one M0 smoke test with both OFF. Use separate build trees for those configurations. `audio_capture` creates a deterministic ALSA file-over-null source, checks the complete WAV against its PCM samples, and exercises diagnostics and cleanup without a microphone. The M0 helpers use a shared `build/` cache. [Host fixture](../../../tests/audio_capture_test.py) · [M0 build helper](../../../tools/scripts/build.sh) · [M0 test helper](../../../tools/scripts/test.sh)

### Raspberry Pi

The verified target is a Pi 3 Model B with 32-bit Raspbian 12 Bookworm `armhf` userspace. Cross-build using GNU `arm-linux-gnueabihf`, binutils, host pkg-config and an absolute path to a matching sysroot with ALSA and libsndfile development files. The Pi needs the matching `/lib/ld-linux-armhf.so.3` loader and runtime libraries, including `libasound.so.2` and `libsndfile.so.1`. For the acceptance procedure it also needs Python 3, the ALSA tools, a working microphone and a WM8960 speaker. [ARM setup guide](../../project/arm-cross-compilation.md) · [Toolchain](../../../tools/cmake/arm-linux-gnueabihf.cmake)

```bash
sysroot=/absolute/path/to/bookworm-armhf-sysroot
arm_build=build-f002-handover-armhf
bash tools/scripts/build-armhf.sh --sysroot "$sysroot" --build-dir "$arm_build" -- \
  -DAI_ROBOT_ENABLE_AUDIO_CAPTURE=ON -DAI_ROBOT_ENABLE_AUDIO_PLAYBACK=ON
file "$arm_build/ai_robot_record_wav"
arm-linux-gnueabihf-readelf -h -A -l -d "$arm_build/ai_robot_record_wav"
```

Expect little-endian ELF32 ARMv7, EABI5 hard-float and the armhf loader. The helper defaults to Release with tests disabled. For an authorized deployment, set the SSH alias to the intended Pi with a trusted host key. Create a private remote directory, preview and copy both binaries, and compare the host and Pi SHA-256 values for each. [ARM build helper](../../../tools/scripts/build-armhf.sh) · [Deploy helper](../../../tools/scripts/deploy.sh)

```bash
ssh_target=your-pi-alias
remote_dir=$(ssh -o StrictHostKeyChecking=yes "$ssh_target" \
  'mktemp -d "$HOME/ai-robot-f002.XXXXXXXX"')
printf 'Pi working directory: %s\n' "$remote_dir"
for binary in ai_robot_record_wav ai_robot_play_wav; do
  bash tools/scripts/deploy.sh --dry-run "$arm_build/$binary" "$ssh_target" "$remote_dir/$binary"
  bash tools/scripts/deploy.sh --execute "$arm_build/$binary" "$ssh_target" "$remote_dir/$binary"
  sha256sum "$arm_build/$binary"
  ssh -o StrictHostKeyChecking=yes "$ssh_target" "sha256sum -- '$remote_dir/$binary'"
done
```

### Minimal target acceptance

On the Pi, run `arecord -l` and `aplay -l` to confirm capture and playback device 0 on `wm8960soundcard`. Use the configured PCM in each direction. In the deployment directory, run the sequence below with someone ready to speak and listen. Speak a short phrase with a recognizable final word after the “Front Center” cue. The subshell stops at the first failing command. [Phase 3 procedure](phase3-execution-debug.md#3-target-execution) · [Full G02 oracle](phase4-verification-report.md#g02-procedure-and-acceptance-oracle)

```bash
(
  set -e
  pcm='plughw:CARD=wm8960soundcard,DEV=0'
  output_dir=$(mktemp -d ./take.XXXXXXXX)
  wav="$output_dir/capture.wav"
  timeout -k 2s 20s ./ai_robot_play_wav --device "$pcm" /usr/share/sounds/alsa/Front_Center.wav
  timeout -k 2s 20s ./ai_robot_record_wav --device "$pcm" "$wav"
  python3 - "$wav" <<'PY'
import sys
import wave
from pathlib import Path

path = Path(sys.argv[1])
with path.open("rb") as stream:
    header = stream.read(12)
    assert header[:4] == b"RIFF" and header[8:] == b"WAVE", header
    assert int.from_bytes(header[4:8], "little") + 8 == path.stat().st_size
with wave.open(str(path), "rb") as wav:
    actual = (wav.getcomptype(), wav.getnchannels(), wav.getsampwidth(),
              wav.getframerate(), wav.getnframes(), len(wav.readframes(240001)))
    assert actual == ("NONE", 1, 2, 48000, 240000, 480000), actual
assert set(path.parent.iterdir()) == {path}
print(f"Verified {path}: {actual}")
PY
  timeout -k 2s 20s ./ai_robot_play_wav --device "$pcm" "$wav"
)
```

Expect one published WAV and an intact, intelligible phrase at the expected pace, with no obvious clipping or dropouts. Exit 0 and a valid payload establish file and transport behavior. The listener establishes speech quality. The full G02 procedure also checks RIFF chunk bounds and records artifact identities, mixer state and the exact take's digest. [Engineer listening criteria and results](phase4-verification-report.md#g02--pi-capture-wav-integrity-f001-playback-and-engineer-listening--pass)

## Verified baseline and evidence limits

**VERIFIED by inspection.** The final implementation is commit `dd5f8940aaf256d16fde168101b2664f988e8d7c`. The corrected ARM artifact's saved SHA-256 matches `050b265b1eeb30b0112dc5027c9a91b90bd65f3c8ab5c60b76f9ea72895b8945`. **HISTORICAL.** Phase 4 was approved on 2026-11-08. The following results come from earlier runs. [Phase 4 decision](phase4-verification-report.md#6-final-assessment--handoff)

| Group | Oracle and observed result |
| --- | --- |
| **G01 · Host regression** | Both OFF passed 1/1 with the original M0 message and no audio target. Capture-only passed 2/2, including a byte-for-byte match of all 480,000 sample bytes, the intended error stages and exit codes, sentinel preservation and directory cleanup. Both ON passed 4/4, including F001 regressions. The corrected CLI passed the same capture-only and both-ON checks, and the owner repeated the OFF check successfully. [Phase 4 host results](phase4-verification-report.md#g01--linux-build-and-existing-host-regression--pass) · [Corrected host log](../../../build-f002-phase4b/host/both-sigxfsz-retest.log) |
| **G02 · Pi recording and F001 replay** | The original ARM recorder from `c0bd8fcf85abb85eb709aa4d504ecf2e563694f5` produced one complete WAV with the expected RIFF fields and all 480,000 payload bytes. Capture and replay both exited 0. The Engineer confirmed intelligible speech, normal pace, an intact ending with “lantern,” and no obvious clipping or dropouts. The corrected ARM binary has static ABI evidence. Its Pi runtime evidence remains pending. [Target log](../../../build-f002-phase4b/g02/target.6BDcEl.log) · [Corrected ARM inspection](../../../build-f002-4a01/armhf.log) |
| **G03 · File-size limit** | The original recorder was terminated by `SIGXFSZ` and left a 32 KiB temporary sibling. Under the same child-only 32 KiB limit and default, unblocked signal disposition at process start, the corrected CLI exited 1, reported `write WAV` and `File too large`, and left the output directory empty. [Original log](../../../build-f002-phase4b/host/g03.log) · [Independent retest](../../../build-f002-phase4b/host/g03-sigxfsz-retest.log) |

**DEFERRED.** Real hardware overruns, short positive and zero-progress reads, device-busy timing, late publication races and finalization faults lack focused runtime coverage. Listening applies to the tested microphone and mixer route. Local logs in ignored build directories may be absent from a fresh checkout. The Phase 4 report records their results and scope. [Residual risks and approval](phase4-verification-report.md#6-final-assessment--handoff)

## Debugging and maintenance

- **File-size failures.** The existing host write-error test deliberately ignores `SIGXFSZ`, so it exercises the returned-error path. The original G03 reproduced termination under the default disposition. Installing `SIG_IGN` in the CLI lets the writer return a failure and scoped cleanup run. A direct API caller owns its process signal policy. The capture library leaves that policy to its host process. [CLI correction and retest](phase4-verification-report.md#retest-of-the-updated-recorder-cli--2026-11-08)
- **Quiet or distorted speech.** Check the capture endpoint and existing mixer state with `arecord -L` and `amixer -c wm8960soundcard scontents`. Compare an independent `arecord` take and replay a known-good cue to isolate input from output. The verified stereo-to-mono route averages both ADC channels, so input routing and gain matter even with a valid WAV. The preflight recorded 17 negative full-scale samples and clear speech. Sample extrema support diagnosis, while listening supplies the acceptance verdict. [Hardware preflight and commands](phase1-requirement-design.md#hardware-preflight)
- **Transfer regressions.** The initial short-source fixture assumed the ALSA file plugin exposed source EOF, but it reported the slave's frame count. The tracked test now induces a read error with a failing `/dev/full` sink. Keep the independent PCM byte comparison and known tail when changing the frame loop. [Phase 2 test lesson](phase2-implementation-build.md#4-tests) · [Capture fixture](../../../tests/audio_capture_test.py#L54)
- **Changing capture duration or adding a PCM consumer.** The frame budget is defined next to the format constants and used by the transfer loop and final readback. A duration change needs consistent frame limits and fixtures. Preserve short-read accounting, atomic publication, the existing five-second behavior for existing callers, and F001 compatibility. The loop owns and reuses its buffer, so a future streaming consumer needs an explicit frame ownership and lifetime contract. [Capture constants and transfer](../../../software/src/audio/wav_capture.cpp#L26) · [F001 handover](../F001-audio-playback/technical-handover.md)

## References

[Feature overview](overview.md) · [Phase 1 design](phase1-requirement-design.md) · [Phase 2 build and function inventory](phase2-implementation-build.md) · [Sequence diagram](phase2-implementation-design.puml) · [Phase 3 target evidence](phase3-execution-debug.md) · [Phase 4 verification](phase4-verification-report.md)
