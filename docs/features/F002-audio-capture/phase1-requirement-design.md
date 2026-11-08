# F002 — Audio Capture

**Phase 1 — Requirement & Design · APPROVED / PASS · 2026-11-08**

[Feature overview](overview.md)

## 0. Project Context & Vision

The long-term goal is a Conversational AI Robot that can converse, understand context, and act. The first MVP is a Voice Command Robot that listens, recognizes simple commands, acts, and responds. F002 gives the Audio subsystem a local microphone input. F001 consumes the resulting WAV for listening verification. A future speech recognizer can consume the PCM frames, and the Robot Orchestrator can decide when to listen, pass audio to recognition, and play a response. The session notes record the subsystem split and the plan to build subsystem functions before adding orchestration. The vision, roadmap, and backlog are still drafts. [Architecture discussion](../../sessions/2026/2026-11-03.md) · [Vision](../../project/vision.md) · [Roadmap](../../project/roadmap.md) · [Backlog](../../project/backlog.md)

## 1. Feature Understanding

The Engineer approved the requirements on 2026-11-08. The WM8960 preflight satisfied the R2 format condition.

| ID | Requirement |
| --- | --- |
| R1 | A C++17 foreground CLI captures the microphone routed through the Pi's WM8960. The caller supplies the ALSA capture PCM and a destination path at invocation. |
| R2 | Capture exactly 240,000 mono frames at 48,000 Hz in signed 16-bit PCM. This is 5.000 seconds of audio and 480,000 bytes of sample data. The frame count defines duration. |
| R3 | On success, publish one readable little-endian RIFF/WAVE PCM file at a new destination and exit 0. An existing destination is preserved. Report usage errors with exit 2 and recording or output errors with exit 1 and a stage-specific diagnostic. A reported capture or write failure leaves no partial WAV at the destination. |
| R4 | The Engineer speaks a short phrase into the intended microphone, then plays the recorded file with F001 through the WM8960. Require intelligible speech at the expected pace, with the spoken ending intact and no obvious clipping or dropouts. |
| R5 | Build capture as a Linux-only, opt-in target. With both audio options OFF, the M0 executable and smoke test run without audio development libraries. |

The MVP covers one recording using the existing microphone and mixer setup. Continuous PCM streaming, VAD, ASR/STT, simultaneous playback and capture, acoustic echo cancellation, and AI, Motion, and Robot Orchestrator implementation are out of scope.

## 2. Context & Information

| Label | Evidence or gap | Effect on F002 |
| --- | --- | --- |
| ENGINEER_PROVIDED | WM8960 capture previously worked. The brief identifies the WM8960 microphone path and proposes PCM16 mono at 48 kHz for five seconds. | Treat prior success as a lead and the format as the preflight target. |
| VERIFIED | F001's historical playback baseline records Pi 3 Model B, Raspbian 12 Bookworm, the `wm8960-soundcard` overlay, card ID `wm8960soundcard`, and PCM device 0 for playback. The Engineer confirmed 48 kHz mono WAV playback in the F001 verification matrix. [Playback baseline](../F001-audio-playback/phase1-requirement-design.md#hardware-preflight) · [Verified listening results](../F001-audio-playback/phase4-verification-report.md#mvp-02--raspberry-pi--wm8960-listening--pass) | Provides a candidate capture card and a compatible WAV reader for later listening acceptance. |
| VERIFIED | A search of the repository docs, audio source, Git history for capture-tool references, and available F001 preflight records found no saved WM8960 capture command, capture result, or input mixer snapshot. The F001 mixer evidence concerns output switches. [F001 baseline](../F001-audio-playback/phase1-requirement-design.md#hardware-preflight) | Current input readiness needs a separate preflight. |
| VERIFIED | F001 uses an explicit PCM, a callable Audio function, a thin CLI, scoped ALSA and libsndfile handles, an opt-in CMake target, and CTest on the Linux host. The ARM build guide records a Bookworm armhf sysroot with both libraries. [CLI](../../../software/src/audio/play_wav_main.cpp) · [Playback code](../../../software/src/audio/wav_playback.cpp) · [CMake](../../../CMakeLists.txt) · [ARM guide](../../project/arm-cross-compilation.md#prepare-the-target-sysroot) | Reuse the conventions, dependencies, and build path. Playback transfer and drain remain direction-specific. |
| VERIFIED | The Pi enumerated WM8960 device 0 for capture and playback. In capture, `plughw:CARD=wm8960soundcard,DEV=0` delivered mono S16_LE at 48 kHz by averaging the two hardware channels. [Preflight evidence](#observed-preflight-evidence) | The selected route delivers the approved application-facing format. |
| ENGINEER_PROVIDED | The Engineer confirmed readiness at the intended microphone and WM8960 speaker for the preflight. | The exact microphone model and connector were not recorded. The listening result establishes whether the test recording contains usable speech. |
| DEFERRED | Streaming block delivery, ASR input format, and playback/capture scheduling. | Revisit when those callers have concrete requirements. |

### Hardware preflight

**Test date · 2026-11-08.** The Engineer authorized the capture and playback check using the existing Pi configuration. The working input route needs the connected microphone's analog input routed to an ADC, the relevant capture channel unmuted, usable gain, and microphone bias if the hardware requires it. The Raspberry Pi overlay describes a mic input and the codec driver exposes input routing, `Capture` and `ADC PCM` controls. The board and microphone determine the actual route and settings. [Raspberry Pi WM8960 overlay](https://raw.githubusercontent.com/raspberrypi/linux/rpi-6.12.y/arch/arm/boot/dts/overlays/wm8960-soundcard-overlay.dts) · [WM8960 codec driver](https://raw.githubusercontent.com/raspberrypi/linux/rpi-6.12.y/sound/soc/codecs/wm8960.c)

1. Identify the capture PCM with `arecord -l` and `arecord -L`. With the card ID reported by `arecord -l`, save `amixer -c wm8960soundcard scontents` using the actual ID if it differs. Use `amixer -c wm8960soundcard contents` if the simple view omits input routing. Record the intended microphone and connection. Confirm `arecord`, `aplay`, and a WAV metadata reader are available for this check.
2. If enumeration confirms the candidate card and device, run the commands below. Use a fresh scratch directory and substitute the actual card and device if needed. Wait for the Engineer to submit **Start**. Begin immediately with the known-good `Front_Center.wav` cue on the WM8960 speaker, then open capture. The Engineer pauses one beat after the cue, says a distinct phrase with a recognizable final word, and listens to the replay. Save verbose output, exact PCM, and exit status. `--fatal-errors` exposes capture overruns as failures and the timeout bounds this diagnostic run. [Bookworm `arecord` options](https://manpages.debian.org/bookworm/alsa-utils/arecord.1.en.html)
3. Inspect the WAV with Python's `wave` module or an equivalent reader. Check PCM encoding, 1 channel, 2 bytes per sample, 48,000 Hz, 240,000 frames, and a complete 480,000-byte payload. After recording stops, use `aplay -D 'plughw:CARD=wm8960soundcard,DEV=0' "$wav"` with the confirmed **playback** PCM. Record its exit status and the Engineer's listening result. If audibility is ambiguous, replay a known-good F001 fixture through the same output to distinguish a capture problem from a speaker route problem. [F001 reference fixture](../F001-audio-playback/phase1-requirement-design.md#hardware-preflight)

```bash
# In an authorized Pi session, after confirming the capture endpoint.
pcm='plughw:CARD=wm8960soundcard,DEV=0'
scratch=$(mktemp -d) && wav="$scratch/f002-preflight.wav" &&
    timeout 15s arecord -v --fatal-errors -D "$pcm" -t wav -f S16_LE -c 1 -r 48000 -d 5 "$wav"
printf 'capture exit %s\n' "$?"
```

| Result | Oracle |
| --- | --- |
| PASS | The selected capture PCM is backed by the enumerated WM8960 endpoint. `arecord` exits 0 without an xrun or capture error. The independent WAV check matches R2 and reads the full payload. `aplay` exits 0 and the Engineer recognizes the phrase and its ending from the intended microphone at the expected pace, with no obvious clipping or dropouts. |
| FAIL | The required capture endpoint is absent, capture errors or times out, WAV checks fail, or the recorded speech is missing or has an audible defect after the playback route is confirmed. Capture the failing stage and evidence for Engineer review. |
| Incomplete | Access, tools, the intended microphone, or a usable playback check is unavailable. Keep Phase 2 gated until the missing evidence is collected. |

If the mono route opens but speech is quiet or missing, a short stereo capture on the same card is the proposed follow-up for a separately approved experiment. Compare left and right signal and mixer routing. If the hardware needs an explicit channel map, review an Engineer-approved ALSA PCM route before considering any change to R2. Treat mixer changes, Pi configuration changes, and revisions to the MVP format as separate Engineer decisions supported by the observed failure.

### Observed preflight evidence

The Engineer submitted **Start** before the accepted take. `aplay` played the known-good “Front Center” clip as a cue, then the five-second capture opened. The earlier timed take met the transfer checks and had no listening verdict.

| Check | Actual observation |
| --- | --- |
| Target and tools | `uname -srmo` reported `Linux 6.12.87+rpt-rpi-v7 armv7l GNU/Linux` and `/etc/os-release` reported Raspbian GNU/Linux 12 (Bookworm). `arecord` reported 1.2.8. `aplay`, `amixer`, `python3`, and `timeout` were available. |
| Capture and playback PCM | `arecord -l` and `aplay -l` both identified card 2 `wm8960soundcard`, device 0, `bcm2835-i2s-wm8960-hifi wm8960-hifi-0`. `arecord -L` listed `plughw:CARD=wm8960soundcard,DEV=0`. Both verbose runs confirmed the same hardware endpoint. |
| Existing mixer | `amixer -c wm8960soundcard scontents` showed both Capture switches on at 39/63 (+12 dB), both ADC PCM controls at 195/255 (0 dB), and the ADC data outputs mapped left to left and right to right. LINPUT1 and RINPUT1 boost routes and both input mixer boost switches were on. LINPUT1 and RINPUT1 boost gains were +29 dB. ALC and noise gate were off, and both output PCM mixer switches were on. The check used the existing mixer settings. |
| Capture transfer | `arecord` exited 0 with no xrun or capture diagnostic. The client used interleaved S16_LE mono at exactly 48,000 Hz. The hardware used S16_LE stereo at the same rate, with a 6,000-frame period and a 24,000-frame buffer. ALSA printed the capture route `0 <- 0*0.5 + 1*0.5`. |
| WAV integrity | Python `wave` exited 0 after checking uncompressed PCM, 1 channel, 2 bytes per sample, 48,000 Hz, 240,000 frames (5.000 s), and all 480,000 payload bytes. PCM sample range was −32,768 to 28,663, with 17 samples at negative full scale. |
| Playback transfer | `aplay` exited 0 with no playback diagnostic. ALSA copied the mono stream to both hardware channels at 48,000 Hz. |
| Listening | ENGINEER_PROVIDED · The Engineer selected “Yes, the full phrase sounded clear” when asked about pace, the final word, clipping, and dropouts. |

```bash
pcm='plughw:CARD=wm8960soundcard,DEV=0'
wav='/tmp/tmp.JfbzkCA5bm/f002-preflight.wav'
# Start after the Engineer submits Start. Play the cue before opening capture.
timeout 15s aplay -q -D "$pcm" /usr/share/sounds/alsa/Front_Center.wav
timeout 15s arecord -v --fatal-errors -D "$pcm" -t wav -f S16_LE -c 1 -r 48000 -d 5 "$wav"
# WAV metadata and the complete sample payload were checked before playback.
timeout 15s aplay -v -D "$pcm" "$wav"
```

The Pi scratch WAV SHA-256 was `3784410fdc9de3797ac47c90047f5fff9fb4eabc2139e9f4a8d05deded57ad30`.

**Preflight PASS.** The confirmed WM8960 route delivered the required WAV, and the Engineer confirmed the full phrase sounded clear. The approved listening oracle was met. The full-scale sample count remains part of the recording evidence.

## 3. Technical Design

**CLI** · `ai_robot_record_wav --device <alsa-pcm> <path.wav>`

**Internal Audio API** · `CaptureResult record_wav_file(const std::filesystem::path& path, const std::string& pcm_device)`

The CLI checks arguments and maps a diagnostic-bearing `CaptureResult` to the exit status, following F001. The Audio function owns the capture session and output lifecycle. Keep the fixed sample rate, channel count, and frame budget inside the capture module. [F001 API and result convention](../F001-audio-playback/phase2-implementation-build.md#7-function-inventory)

```text
Microphone → WM8960 analog input and ADC → explicit ALSA capture PCM
           → reusable PCM16 frame buffer → libsndfile → finalized WAV
           → F001 playback → Engineer listening check
```

1. Validate the destination and create a secure temporary sibling file before recording. Open the supplied PCM with `snd_pcm_open` in blocking capture mode. Require native-endian S16, interleaved mono, at exactly 48 kHz at the application boundary. Fail configuration if the selected PCM cannot provide that stream. Use the configured PCM's negotiated period to size one reusable buffer. The `plughw` route may adapt hardware parameters. Accept it only if the preflight demonstrates that the resulting mono stream contains the intended microphone signal. [ALSA PCM functions](https://www.alsa-project.org/alsa-doc/alsa-lib/group___p_c_m.html) · [ALSA plug conversion](https://www.alsa-project.org/alsa-doc/alsa-lib/pcm_plugins.html)
2. Use `snd_pcm_readi` to request up to one period or the remaining frame budget, whichever is smaller. Write the actual positive count to libsndfile with `sf_writef_short` and advance by that count. Treat zero progress, negative reads including capture overruns, and short or failed WAV writes as errors. Finish at exactly 240,000 written frames. The file contains only captured frames. [ALSA capture and overrun behavior](https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html) · [libsndfile frame API](https://libsndfile.github.io/libsndfile/api.html)
3. Stop capture with `snd_pcm_drop` after the frame budget and release the PCM. Report a stop failure before publishing. Finalize the `SF_FORMAT_WAV | SF_FORMAT_PCM_16` writer and check `sf_close` before publication. Use a same-directory exclusive temporary file and an atomic no-replace publish operation such as `renameat2` with `RENAME_NOREPLACE` on a supported local filesystem. Scoped cleanup releases handles and removes the temporary file on handled failures. Report the stage, PCM or path, and cause. [libsndfile close and file descriptor APIs](https://libsndfile.github.io/libsndfile/api.html) · [Linux rename semantics](https://man7.org/linux/man-pages/man2/rename.2.html)

Use ALSA/libasound and libsndfile directly. `arecord` and `aplay` provide an independent baseline. The alternatives of wrapping `arecord` or manually writing WAV headers would introduce process orchestration or container maintenance for behavior already covered by the F001 dependencies. The frame loop is the extension point for a later PCM consumer. A later feature can define a streaming interface around that loop once an ASR or orchestration caller exists.

| Planned location | Responsibility |
| --- | --- |
| `software/src/audio/wav_capture.hpp` and `.cpp` | Callable Audio function, blocking PCM setup and frame transfer, WAV finalization, and cleanup. |
| `software/src/audio/record_wav_main.cpp` | CLI arguments, exit status, and diagnostic output. |
| `CMakeLists.txt` | `AI_ROBOT_ENABLE_AUDIO_CAPTURE`, default OFF, and `ai_robot_record_wav`. Reuse `ALSA::ALSA` and `PkgConfig::SNDFILE` discovery when either audio feature is enabled. |

## 4. Execution Plan

1. Enter Phase 2 after a passing preflight, Engineer approval of the final design, and explicit authorization to begin implementation. Confirm the development libraries and `pkg-config` in the Linux build environment and the existing compatible armhf sysroot. On the Pi, the relevant runtime libraries are libasound and libsndfile. [F001 build precedent](../F001-audio-playback/phase2-implementation-build.md#2-build) · [ARM guide](../../project/arm-cross-compilation.md)
2. Add the opt-in target, CLI, and Audio function. Review frame accounting, writer finalization, and failure cleanup, then run focused host checks and the existing regressions in Section 5. Use the existing ARM build script with the capture target and inspect the linked artifact against the target ABI. [ARM build script](../../../tools/scripts/build-armhf.sh)
3. Schedule target execution for Phase 3 after authorization. Use the preflight PCM and a fresh destination. Record the command, exit status, WAV metadata, and the Engineer's F001 playback observation. Use the resulting evidence for the independent Phase 4 review.

## 5. Verification & Risks

| Check | Acceptance and evidence |
| --- | --- |
| Hardware baseline · R1–R2 | Apply the [preflight oracles](#hardware-preflight). Retain device enumeration, mixer readout, capture and playback commands, exit statuses, WAV metadata and payload length, and the Engineer's listening result. |
| Host transfer · R2–R3 | Supply a deterministic 240,000-frame raw PCM16 source with distinct samples and a recognizable tail, for example through an ALSA file PCM with `infile` over `null` in a disposable test configuration. Read the generated WAV independently, assert the R2 format and size, and compare the entire payload against the source. This detects lost, repeated, or padded frames. Confirm fixture support on the host before relying on it. [ALSA file PCM](https://www.alsa-project.org/alsa-doc/alsa-lib/pcm_plugins.html) |
| Host errors · R3 | Missing arguments yield exit 2. An unknown PCM or unusable destination yields exit 1 with the expected stage and leaves no destination WAV. A pre-existing file yields exit 1 and retains its original bytes. Check for temporary-file cleanup on a failure after staging and review overrun, short write, and finalization error paths. |
| Build isolation · R5 | With both audio options OFF, build M0 and pass its smoke test without requiring audio packages. With capture ON, build the capture CLI and run its focused checks. With both audio options ON, run the existing F001 host suite to cover shared dependency discovery. [CMake](../../../CMakeLists.txt) · [F001 host checks](../../../tests/audio_playback_test.py) |
| Raspberry Pi acceptance · R1–R4 | Run the F002 CLI against the confirmed WM8960 capture PCM. Expect exit 0 and one finalized WAV at the new path. Independently confirm the format and full payload, then use `ai_robot_play_wav --device <confirmed-playback-pcm> <recorded.wav>` after capture exits. Expect exit 0 and Engineer confirmation of R4. Retain the invocation, exit statuses, WAV evidence, and listening result. |

- **Capture is silent or distorted despite a successful transfer.** Capture routing, gain, and mono conversion can affect the signal. Require an audible phrase on the preflight and the application recording.
- **Capture overruns or the device is busy.** Use blocking reads and the negotiated period, surface ALSA errors with the selected PCM, and fail the recording on discontinuity. A later recording can use a fresh destination after the cause is resolved.
- **Short writes or a publication error.** Check write counts and finalization before publishing. Atomic no-replace publication protects the previous destination even if another process creates it during capture. Abrupt termination can leave the temporary file for cleanup.

## 6. Decision & Readiness

**APPROVED / PASS.** The Engineer gave final Phase 1 approval on 2026-11-08 after the WM8960 preflight passed and satisfied the D1 condition. Phase 2 awaits separate Engineer authorization.

| ID | Engineer decision |
| --- | --- |
| D1 · Hardware and format | Approve five seconds of mono PCM16 WAV at 48 kHz, subject to a passing WM8960 preflight. |
| D2 · Entry point and libraries | Use ALSA/libasound and libsndfile in a callable C++17 function with an explicit-device CLI and an opt-in build target. |
| D3 · Output and error policy | Publish a finalized WAV to a fresh destination, preserve existing files, and fail on capture discontinuity or output error. |
| D4 · Acceptance | Use the hardware baseline, focused host checks, and Engineer listening verification of a Pi recording played through F001. |

Next Engineer action. Authorize Phase 2 when ready.
