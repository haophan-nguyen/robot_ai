# F001 — Audio Playback Technical Handover

F001 gives the Voice Command Robot a way to play prerecorded prompts and acknowledgments. The C++17 player plays one local file once, in the foreground, through the ALSA PCM supplied by its caller. The MVP route is the Raspberry Pi's WM8960. Playback has a standalone CLI and a callable C++ function. The M0 executable has its own entry point, and future orchestration can call the playback function to play a prompt. [Approved scope](phase1-requirement-design.md#1-feature-understanding) · [Subsystem context](../../sessions/2026/2026-11-03.md)

The supported input is a readable regular file containing nonempty little-endian RIFF/WAVE PCM16, including WAVE_FORMAT_EXTENSIBLE when libsndfile identifies the subtype as signed 16-bit PCM. Supported source formats are mono or stereo at 44.1 or 48 kHz. Metadata determines acceptance regardless of filename suffix. libsndfile may normalize malformed metadata, which limits strict RIFF integrity checks. Other encodings, mixer or device configuration, queues, mixing, cancellation, capture concurrency, automatic underrun recovery and streaming TTS are outside F001. [Approved design](phase1-requirement-design.md#3-technical-design) · [Format validation](../../../software/src/audio/wav_playback.cpp#L30)

## Entry points and contracts

| Entry or function | Contract and responsibility |
| --- | --- |
| [CLI main](../../../software/src/audio/play_wav_main.cpp#L6) | Takes exactly `ai_robot_play_wav --device <alsa-pcm> <path.wav>`, with both values nonempty. Returns 2 and writes usage to stderr for invalid arguments, 1 and writes the playback diagnostic to stderr on playback failure, or 0 with no application output after successful transfer and drain. |
| [Playback API](../../../software/src/audio/wav_playback.hpp#L8) · [Implementation](../../../software/src/audio/wav_playback.cpp#L112) | `ai_robot::audio::play_wav_file(const std::filesystem::path& path, const std::string& pcm_device)` returns `PlaybackResult`. The path is local and the PCM name must be explicit. `explicit operator bool() const noexcept` is true when `diagnostic` is empty. This call blocks and owns its file and PCM handles. |
| [validate_wav_format](../../../software/src/audio/wav_playback.cpp#L30) | Accepts `SF_INFO` from libsndfile and returns an empty string on success or a reason for rejection. This is the format policy boundary. |
| [transfer_frames](../../../software/src/audio/wav_playback.cpp#L68) · [write_frames](../../../software/src/audio/wav_playback.cpp#L50) | `transfer_frames` takes the open handles, WAV metadata, diagnostic subjects and negotiated period in frames. It decodes and submits the stream. `write_frames` takes the PCM, its name, the sample pointer, pending frame count and channel count. It completes one chunk across as many positive writes as needed. Both return a `PlaybackResult`. |
| [CMake targets and tests](../../../CMakeLists.txt) · [M0 entry point](../../../software/src/main.cpp) | The Linux-only option `AI_ROBOT_ENABLE_AUDIO_PLAYBACK` defaults to OFF. When enabled, the CLI compiles the playback module and links `ALSA::ALSA` and `PkgConfig::SNDFILE`. Native builds with `BUILD_TESTING=ON` also register the Python audio tests. |

## From file to speaker

1. **Validate before opening the device.** The CLI checks syntax and calls `play_wav_file`. The function rejects an empty PCM name or a nonregular path, then opens the WAV with `sf_open` in read mode. `validate_wav_format` checks the container, endianness, subtype, channels, sample rate and positive frame count from libsndfile before ALSA is opened. A rejected file therefore cannot claim the playback device.
2. **Configure the chosen route.** The function opens only the named PCM, in blocking playback mode. `snd_pcm_set_params` requests native-endian signed 16-bit samples, read/write interleaved access, the file's channel count and sample rate, software resampling enabled and 200 ms latency. The selected `plughw` route can adapt that stream for the hardware. `snd_pcm_get_params` returns the buffer and period sizes in frames. The negotiated period `P` determines the application's chunk size. The 200 ms request is for latency, while chunk duration depends on the negotiated period and sample rate.
3. **Decode one period at a time.** A frame holds one sample per channel. Stereo stores `[L0, R0, L1, R1, …]`. `sf_readf_short` decodes into native `short` samples, and `transfer_frames` allocates one reusable `P × channels` sample buffer with size and allocation checks. Each read requests the lesser of `P` and the remaining WAV frames. A decoder error or a read shorter than requested fails before that chunk is submitted.
4. **Submit complete chunks.** A successful final read shorter than `P` is zero-filled from `decoded × channels` through the end of the buffer. For `N` WAV frames, the added silence is `(P - N % P) % P` frames, so aligned input adds zero. `snd_pcm_writei` takes and returns frame counts. After a positive partial write the loop advances the pointer by `written × channels` samples and reduces the pending count by `written` frames. A negative write, including an underrun, or zero progress fails. The WAV frame counter advances only by decoded frames.
5. **Drain and release.** The app calls `snd_pcm_drain` after successful submission so queued frames can finish playing. A transfer or drain failure attempts `snd_pcm_drop` and returns the original stage-specific diagnostic with the file or PCM name and reason. An ALSA open, configuration or parameter-query failure returns at its own stage. Scoped deleters close acquired PCM and WAV handles on return, including error paths.

[Playback implementation](../../../software/src/audio/wav_playback.cpp) · [Implementation flow diagram](phase2-implementation-design.puml)

## Build and run

Use Bash from the repository root in Linux or WSL. The project needs CMake 3.16+, a C++17 compiler and a build tool. On Ubuntu/Debian, the native audio build also needs `pkg-config`, `libasound2-dev` and `libsndfile1-dev`. Native audio tests need Python 3. The historical Linux runs used Ubuntu 22.04 WSL and GCC 11.4.0. The commands below are for reproduction. Phase 5 used existing logs for runtime evidence. [Phase 2 build](phase2-implementation-build.md#2-build) · [Phase 4 host commands](phase4-verification-report.md#mvp-01--build-and-existing-software-regression--pass)

### Linux host

```bash
cmake -S . -B build-f001-handover-host -DCMAKE_BUILD_TYPE=Debug \
  -DAI_ROBOT_ENABLE_AUDIO_PLAYBACK=ON -DBUILD_TESTING=ON
cmake --build build-f001-handover-host --parallel
(cd build-f001-handover-host && ctest --output-on-failure -V)
```

Expect `ai_robot_smoke`, `audio_playback_valid` and `audio_playback_invalid` to pass. The tests generate their own WAVs and use ALSA `null` for software playback and a file PCM to capture submitted samples. If the account can read mode-000 files, the unreadable-file subcase prints `SKIP`. To check a supported local WAV manually, run `./build-f001-handover-host/ai_robot_play_wav --device null /path/to/clip.wav` and expect exit 0 with no application output. [Host test source](../../../tests/audio_playback_test.py)

For an audio-OFF build, use a separate build tree with `-DAI_ROBOT_ENABLE_AUDIO_PLAYBACK=OFF`. Expect only `ai_robot_smoke` and the message `AI Robot: M0 foundation ready.` The M0 build and test scripts use the shared `build/` cache. [M0 build script](../../../tools/scripts/build.sh) · [M0 test script](../../../tools/scripts/test.sh)

### Raspberry Pi and WM8960

The verified target was a Raspberry Pi 3 Model B running 32-bit Raspbian 12 Bookworm with `armhf` userspace. Use the GNU `arm-linux-gnueabihf` toolchain, binutils, host CMake and pkg-config, and a matching Bookworm armhf sysroot with the ALSA and libsndfile development files. The toolchain restricts library and package discovery to that sysroot. On the Pi, the executable needs the armhf loader `/lib/ld-linux-armhf.so.3`, matching runtime libraries including `libasound.so.2` and `libsndfile.so.1`, and a working WM8960 route. Check `aplay -l` and `aplay -L` on the Pi for the configured card and PCM. The verified route identifies the card by name. [ARM setup guide](../../project/arm-cross-compilation.md) · [Toolchain](../../../tools/cmake/arm-linux-gnueabihf.cmake) · [Hardware preflight](phase1-requirement-design.md#hardware-preflight)

```bash
sysroot=/absolute/path/to/bookworm-armhf-sysroot
arm_build=build-f001-handover-armhf
bash tools/scripts/build-armhf.sh --sysroot "$sysroot" --build-dir "$arm_build" \
  --target ai_robot_play_wav -- -DAI_ROBOT_ENABLE_AUDIO_PLAYBACK=ON
player="$arm_build/ai_robot_play_wav"
file "$player"
arm-linux-gnueabihf-readelf -h -A -l -d "$player"
```

Expect little-endian ARMv7 ELF32, EABI5 hard-float and the armhf loader. The build helper defaults to Release with tests disabled. Use a fresh `--build-dir` when changing toolchains or sysroots. For an authorized transfer, set `ssh_target` to your configured SSH alias or `user@host` with a trusted host key. The deployment helper previews the copy by default and preserves executable mode when copying. [Build helper](../../../tools/scripts/build-armhf.sh) · [Deployment helper](../../../tools/scripts/deploy.sh)

```bash
ssh_target=your-pi-alias
remote_dir=$(ssh -o StrictHostKeyChecking=yes "$ssh_target" \
  'mktemp -d "$HOME/ai-robot-f001.XXXXXXXX"')
remote_player="$remote_dir/ai_robot_play_wav"
bash tools/scripts/deploy.sh --dry-run "$player" "$ssh_target" "$remote_player"
bash tools/scripts/deploy.sh --execute "$player" "$ssh_target" "$remote_player"
sha256sum "$player"
ssh -o StrictHostKeyChecking=yes "$ssh_target" "sha256sum -- '$remote_player'"
```

After the digests match, use this minimal target check with the listener ready. The reference `Front_Center.wav` on the verified image is mono PCM16 at 48 kHz, with 68,545 frames. If it is absent, use a known supported fixture with a recognizable ending. Expect exit 0 and audible playback at the expected pace with an intact ending. The full matrix uses one recognizable fixture for each channel/rate pair, played in succession through the same PCM. [Phase 3 target retest](phase3-execution-debug.md#negotiated-period-chunk-experiment) · [Phase 4 listening matrix](phase4-verification-report.md#mvp-02--raspberry-pi--wm8960-listening--pass)

```bash
ssh -o StrictHostKeyChecking=yes "$ssh_target" \
  "'$remote_player' --device plughw:CARD=wm8960soundcard,DEV=0 /usr/share/sounds/alsa/Front_Center.wav"
```

## Verification baseline and limits

**HISTORICAL · Phase 4 APPROVED / PASS on 2026-11-08.** The approved implementation revision is `79ebd32e1cce29a31c66aeab66ee3dcacdff7db7`. The target run used the Phase 3 ARM artifact with SHA-256 `e60ebc2c06b0f52616ebb0405ced47e9467c1e050ab52ea7d2a37218c332dbc4`, checked on both host and Pi. [Approved Phase 4 report](phase4-verification-report.md#5-4b-execution-evidence)

| Group | Accepted oracle and observed result |
| --- | --- |
| MVP-01 · Host regression | OFF passed 1/1 with the original M0 message and no audio target. ON passed 3/3, including all four formats through `null`, the file captures and all 12 negative assertions. Negative tests compared exits and diagnostic stages, including WAV rejection before PCM open. [OFF log](../../../build-f001-phase4b/host-off.log) · [ON log](../../../build-f001-phase4b/host-on.log) |
| MVP-02 · WM8960 listening | Four successive plays covered 44.1 and 48 kHz mono and stereo. Each exited 0 without diagnostics. The Engineer confirmed audibility, expected pace and intact endings on all four, including the original “Front Center” clip. The test checked the audible sequence and ending on the configured speaker. Stereo separation was outside its scope. [Playback log](../../../build-f001-phase4b/pi-listening-Gx7thR6g) · [Fixture evidence](phase4-verification-report.md#mvp-02--raspberry-pi--wm8960-listening--pass) |
| MVP-03 · Final chunk | The file sink preserved the one-frame stereo input and added only silence. It captured 4,800 aligned source frames byte for byte with zero padding, and 4,801 partial source frames byte for byte plus 2,399 silent frames. Expected PCM bytes came from the generated input. The 2,400-frame chunk size was inferred from the one-frame output. [CTest log](../../../build-f001-phase4b/on/Testing/Temporary/LastTest.log) |

**DEFERRED.** F001-4A-01 covers the lack of an independent negotiated-period oracle. A consistent period mismatch can escape the output-derived expectation. F001-4A-02 covers forced partial writes and decoder/ALSA stream failure and cleanup behavior, which have source-review evidence. The approved plan deferred these two coverage risks. Local logs in the ignored build directories may be absent from a fresh checkout. The Phase 4 report records the accepted results and fixture details. [Phase 4 findings](phase4-verification-report.md#findings)

## Debugging and maintenance

**HISTORICAL.** The original F001 player exited 0 while the Engineer heard the final “-ter” in “Front Center” clip or stutter. The same fixture and PCM sounded clean with `aplay`. The comparison showed both a different buffer/period and additional submitted frames with `aplay`. A silence-padded copy played cleanly with the original F001 binary, but the added silence also aligned the file to a period boundary. The intermediate player added padding to the next boundary plus a full silent period. The approved player uses period-sized chunks and pads only the last incomplete chunk. On the original fixture that reduced the extra frames from 3,455 to 1,055, and the Engineer still heard a clean ending. **UNKNOWN.** The experiment established a mitigation on the tested route. The cause of the original clipping remains unconfirmed. [Incident, comparison and experiments](phase3-execution-debug.md#l1--clipped-or-stuttered-clip-ending)

- **Route or audio regression.** Use the diagnostic stage to distinguish a rejected file from PCM open, configuration, transfer or drain failure. For silent or clipped output with exit 0, compare F001 with `aplay -v -D "$pcm" "$wav"` on the same known WAV and PCM. On an authorized Pi session, capture the artifact identity and negotiated hardware parameters and collect a listening result. Exit 0 reports completed submission and drain. The speaker check establishes audibility and the ending. [Phase 3 comparison](phase3-execution-debug.md#controlled-comparison)
- **New supported source format.** Start with `validate_wav_format` and check that the decoder output and ALSA stream format still agree. If accepting a new rate within PCM16, exercise the new rate through the software sinks and intended hardware PCM while retaining existing positives and an unsupported-rate negative. For a representation change, also revisit frame sizing, interleaving, silence and buffer/ALSA format agreement. Keep the audio-OFF M0 check. [Validation and transfer](../../../software/src/audio/wav_playback.cpp#L30) · [Host fixture suite](../../../tests/audio_playback_test.py)
- **Robot integration.** A caller can reuse `play_wav_file` for compatible on-disk prompts by linking the playback implementation and its dependencies. The current build exposes an executable target. Keep scheduling and PCM selection with the caller, and account for blocking through drain before placing the call on a time-sensitive thread. [API declaration](../../../software/src/audio/wav_playback.hpp) · [Build target](../../../CMakeLists.txt#L28)

## References

[Feature overview](overview.md) · [Phase 1 requirements and decisions](phase1-requirement-design.md) · [Phase 2 implementation and function inventory](phase2-implementation-build.md) · [PlantUML flow](phase2-implementation-design.puml) · [Phase 3 target debugging](phase3-execution-debug.md) · [Phase 4 product verification](phase4-verification-report.md)
