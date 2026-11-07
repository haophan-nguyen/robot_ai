# F001 — Audio Playback

**Phase 1 — Requirement & Design · APPROVED · 2026-11-07**

## 0. Project Context & Vision

The long-term goal is a Conversational AI Robot that understands context and can act. The first MVP is a Voice Command Robot. F001 gives the Audio subsystem a way to play prerecorded prompts and acknowledgments. Audio Capture will provide input, AI may later generate speech, and the Robot Orchestrator can eventually decide when to play a response. The session notes describe those subsystem boundaries and the plan to build subsystems before integrating them. Source: local session note `2026-11-03.md`.

## 1. Feature Understanding

| ID | Requirement |
| --- | --- |
| R1 | The C++17 CLI plays one local WAV through the ALSA PCM supplied in `--device`. WM8960 is the MVP target. Choose the PCM at invocation time without a source change or rebuild. |
| R2 | Accept nonempty RIFF/WAVE with signed 16-bit PCM, mono or stereo, at 44.1 or 48 kHz. Validate the file metadata before opening the PCM. |
| R3 | Play once in the foreground. Use a fixed-size frame buffer and return success after every decoded frame is submitted and the PCM drains successfully. |
| R4 | Return a nonzero exit status and a diagnostic for invalid arguments, an unreadable or unsupported file, a decoder read failure, or an ALSA open, configure, write, or drain failure. Release acquired resources on every return. Use only the selected PCM. |
| R5 | Provide playback as an opt-in build target. The default M0 executable and smoke test must build without audio development libraries and retain their existing behavior. |

Scope covers one blocking playback call using the Pi's existing audio setup. Other encodings and media formats, mixer controls, system audio configuration, concurrent playback and capture, mixing, queueing, cancellation, and streaming TTS are outside F001.

## 2. Context & Information

| Label | Evidence or gap | Effect on F001 |
| --- | --- | --- |
| VERIFIED | M0 has one C++17 executable and one CTest smoke test. The Audio source directory held a placeholder at inspection. [CMake](../../CMakeLists.txt) · [Entry point](../../software/src/main.cpp) · [M0 evidence](../sessions/2026/2026-11-04.md) | F001 supplies the first audio entry point. |
| VERIFIED | The vision, roadmap, and backlog are drafts. The 2026-11-03 session note records the subsystem split and native Pi build direction. The current Engineer brief establishes F001 Playback as the active feature. [Vision](../project/vision.md) · [Roadmap](../project/roadmap.md) · [Backlog](../project/backlog.md) | Use the current brief for scope and the session note for architecture context. |
| VERIFIED | The Pi has `alsa-utils` and `libasound2` at `1.2.8-1+rpt1`, `libsndfile1` at `1.2.0-1+deb12u1`, and `pkg-config` at `1.8.1-1`. `dpkg-query` found neither `libasound2-dev` nor `libsndfile1-dev`. | The development packages are prerequisites for a future native build. |
| UNKNOWN | The other three source channel/rate combinations and playback through the F001 application have no target results. | Track these in application verification. The completed baseline covers only the tested 48 kHz mono source. |
| DEFERRED | Capture/playback duplex behavior, TTS format and streaming needs, and orchestration scheduling. | Revisit when the caller and concurrency requirements exist. |

### Hardware preflight

**Test date:** 2026-11-07

**Baseline PASS for the tested route and fixture.** The Engineer explicitly confirmed hearing the clip through the speaker connected to the WM8960.

| Check | Observed evidence |
| --- | --- |
| Pi and OS | `/proc/device-tree/model` reported Raspberry Pi 3 Model B Rev 1.2. `/etc/os-release` reported Raspbian GNU/Linux 12 (Bookworm). `uname -srmo` reported `Linux 6.12.87+rpt-rpi-v7 armv7l GNU/Linux`. |
| ALSA device | `/proc/asound/cards`, `/proc/asound/pcm`, and `aplay -l` identified `wm8960soundcard` (then card index 2), device 0, `bcm2835-i2s-wm8960-hifi wm8960-hifi-0`. `aplay -L` listed `plughw:CARD=wm8960soundcard,DEV=0`, which was used for the test. |
| Audio configuration | `lsmod` showed the WM8960 codec and sound card modules. `/boot/firmware/config.txt` contained `dtoverlay=wm8960-soundcard`. `/etc/asound.conf` defined `playback` through `plug` and `dmix` backed by the WM8960. `amixer -c wm8960soundcard scontents` showed both left and right output mixer PCM switches on. |
| Fixture | Python's `wave` module validated the installed `/usr/share/sounds/alsa/Front_Center.wav` as signed 16-bit PCM, mono, 48 kHz, 68,545 frames (1.428 s), and read the full sample payload. |
| Result | `aplay` opened the WM8960 hardware PCM at device 0. Its verbose output showed 48 kHz mono input mapped to stereo hardware at 48 kHz. No ALSA error was printed. The wrapper failed to capture the exact `aplay` exit code. The Engineer answered “Yes, I heard it” to the speaker confirmation. |

```bash
aplay -v -D plughw:CARD=wm8960soundcard,DEV=0 /usr/share/sounds/alsa/Front_Center.wav
```

## 3. Technical Design

**CLI** · `ai_robot_play_wav --device <alsa-pcm> <path.wav>`

**Internal interface** · `PlaybackResult play_wav_file(const std::filesystem::path& path, const std::string& pcm_device)`

The CLI validates arguments, passes the PCM name and path to the Audio function, and prints a stage-specific error to stderr on failure. Use exit 0 for successful transfer and drain, 2 for usage errors, and 1 for playback errors. Keep the library handles private to the implementation.

```text
WAV file → libsndfile metadata and PCM frames → ALSA playback PCM
         → WM8960 codec and configured analog output → speaker
```

1. Open the local WAV with libsndfile (D1). Centralize container, encoding, channel count, rate, and positive frame count validation before opening the PCM (D3). Use numeric values for channel count and sample rate. Use enums only for values that benefit from a distinct type. Validate file metadata independently of the extension.
2. Open the caller-supplied PCM with ALSA/libasound in blocking playback mode (D1, D2). Set signed 16-bit native-endian samples, interleaved access, and the file's channel count and rate at the application boundary. Supply `plughw:CARD=wm8960soundcard,DEV=0` on the verified Pi via the CLI. The PCM name, card ID, and device number belong in the invocation. The plug layer can adapt the source stream to the hardware. [ALSA PCM names and devices](https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html)
3. Read fixed-size blocks via `sf_readf_short`. Send the decoded frames with `snd_pcm_writei`. On a partial write, advance the sample pointer by frames written times channel count. Treat zero write progress, negative write results (including underruns), decoder errors, and unexpected EOF relative to the decoder's frame count as failures. The decoder may normalize malformed metadata, which limits strict RIFF integrity checks. [libsndfile frame API](https://libsndfile.github.io/libsndfile/api.html) · [ALSA PCM functions](https://www.alsa-project.org/alsa-doc/alsa-lib/group___p_c_m.html)
4. Drain the PCM after successful transfer to play pending frames. On failure, drop pending frames if playback was active and release both handles via scoped cleanup. Include the path or PCM name and the library error or rejected format in diagnostics. [ALSA PCM functions](https://www.alsa-project.org/alsa-doc/alsa-lib/group___p_c_m.html)

The playback code uses libsndfile and libasound directly. `aplay` is reserved for independent hardware diagnostics and verification (D1, D4).

| Planned location | Responsibility |
| --- | --- |
| `software/src/audio/wav_playback.hpp` and `.cpp` | File playback function, centralized format validation, frame transfer, and cleanup. |
| `software/src/audio/play_wav_main.cpp` | CLI argument handling and diagnostics. |
| `CMakeLists.txt` and `tests/` | Opt-in playback target and focused tests. |

Keep the playback function callable outside the CLI so a future TTS caller can pass a compatible WAV file.

## 4. Execution Plan

Task 1 prepared a Bookworm armhf sysroot with matching ALSA and libsndfile development packages. Task 2 verified the shared ARM build path with the M0 executable. [Project ARM cross-compilation guide](../project/arm-cross-compilation.md)

1. After explicit authorization to start Phase 2, add the Linux-only CMake option `AI_ROBOT_ENABLE_AUDIO_PLAYBACK` (default OFF) and the `ai_robot_play_wav` target. Discover `ALSA::ALSA` through `FindALSA` and the `sndfile` imported module target through `FindPkgConfig` only when playback is enabled. Debian-family build prerequisites include `libasound2-dev`, `libsndfile1-dev`, and `pkg-config`. Confirm the build environment's package availability before installation. [CMake 3.16 FindALSA](https://cmake.org/cmake/help/v3.16/module/FindALSA.html) · [CMake 3.16 FindPkgConfig](https://cmake.org/cmake/help/v3.16/module/FindPkgConfig.html)
2. Implement the centralized file validation, blocking PCM transfer, scoped cleanup, and CLI. Add reproducible WAV fixtures and tests for the approved formats and expected failures.
3. Build and run the software verification on Linux/WSL with the feature enabled. Verify the default M0 build separately. Later target execution requires its own authorization and a permitted Pi session. Use the preflight PCM for the application listening test.

## 5. Verification & Risks

| D4 verification level | Coverage and acceptance |
| --- | --- |
| 1. Software verification · Pending | **R1–R4.** Use `--device null` for all four channel/rate combinations, including a file spanning several reads with a partial final block. Expect exit 0 after frame transfer and drain. Cover missing or unreadable files, invalid headers, empty WAVs, unsupported encodings, rates and channel counts, missing CLI arguments, and an invalid PCM. Expect the corresponding nonzero exit and diagnostic. Pair unsupported files with an invalid PCM to verify that format validation comes first. Review partial-write handling and cleanup on stream failures. **R5.** Build and run the M0 smoke test with playback OFF in an environment without audio development dependencies. With playback ON and dependencies present, build the new CLI and run its focused tests. [ALSA null plugin](https://www.alsa-project.org/alsa-doc/alsa-lib/pcm_plugins.html) |
| 2. Hardware baseline · PASS for the tested fixture | **R1.** The independent `aplay` preflight routed 48 kHz mono PCM through the WM8960. The Engineer heard the clip. The exact `aplay` exit code was unavailable. See [Hardware preflight](#hardware-preflight). |
| 3. Application verification · Pending | **R1–R4.** Run the F001 executable on the Pi with the verified PCM and short clips for all four channel/rate combinations. Record the command, PCM, fixture metadata, exit status, and the Engineer's listening result. Acceptance requires exit 0 and Engineer confirmation that each clip is audible through the WM8960, at expected speed and with its end marker intact. Check the output can be reopened on successive runs. |

- **Routing or silent output.** A PCM transfer can complete while the speaker is silent. The baseline confirms the observed route. If the setup changes or a later check fails, collect evidence and let the Engineer choose the next diagnostic experiment.
- **Format negotiation or device contention.** Use the explicit PCM and report failures. Verify all four source formats on the target. The baseline exercised mono input only, even though ALSA sent stereo frames to the hardware.
- **Dropped frames or clipped endings.** Check transfer counts, partial writes, and drain status. Report underruns as failures. Automatic recovery is deferred.

## 6. Decision & Readiness

The Engineer approved D1–D4 on 2026-11-07.

| ID | Approved decision |
| --- | --- |
| D1 · Libraries | Decode WAV with libsndfile and play PCM through ALSA/libasound. Reserve `aplay` for independent diagnostics and verification. |
| D2 · Device | Target WM8960 for the MVP. Supply the ALSA PCM externally through `--device`. Keep card indexes and PCM identifiers out of the playback logic. The CLI argument provides all configuration required for F001. |
| D3 · WAV scope | One file at a time, blocking playback. Signed 16-bit PCM only, mono or stereo, 44.1 or 48 kHz. Centralize format validation and use numeric parameters for sample rate and channel count. |
| D4 · Acceptance | Verify software behavior with ALSA `null` and negative cases. Establish the WM8960 hardware baseline with `aplay`. Verify the application separately on WM8960 and require the Engineer's audible confirmation in addition to the application's result. |

**Phase 1 APPROVED.** There are no unresolved critical design blockers. Development headers are a future native build prerequisite. Software and application verification remain pending.

**Next action.** Await the Engineer's separate, explicit authorization to start Phase 2. See the local feature-workflow skill and its Phase 1 reference.
