# Phase 4 — Independent Verification & Review Report

**F001 Audio Playback · Phase 4 APPROVED / PASS · 2026-11-08**

[Feature overview](overview.md)

## 0. Review scope & independence

- **Reviewer.** A fresh Codex reviewer session (`independent_review`) inspected F001 separately from the Phase 2/3 implementer. The coordinating Phase 4 session cross-checked the source, test coverage and archived evidence, and compiled this report.
- **VERIFIED baseline.** Reviewed the current worktree on `main` at `79ebd32e1cce29a31c66aeab66ee3dcacdff7db7`, which contains the approved final-chunk change. Before the reporting edits, the scoped diff against HEAD was empty for the feature reports, source, tests, CMake and build/deploy tools. The Phase 4 workflow reference was already present as an untracked file.
- **Reviewed material.** The [approved requirements and design](phase1-requirement-design.md), [Phase 2 report](phase2-implementation-build.md), [implementation diagram source](phase2-implementation-design.puml), [Phase 3 report and incident history](phase3-execution-debug.md), [project subsystem context](../../sessions/2026/2026-11-03.md), [ARM workflow](../../project/arm-cross-compilation.md), and the actual CLI, playback source, CMake, helper scripts and generated-fixture test code linked below.
- **Artifact and evidence limits.** The existing local `build-armhf-f001-chunk/ai_robot_play_wav` has SHA-256 `e60ebc2c06b0f52616ebb0405ced47e9467c1e050ab52ea7d2a37218c332dbc4`, matching the artifact recorded in Phase 3. The saved host `build-audio-host/Testing/Temporary/LastTest.log` from November 8 records three passing tests, including the unreadable-file check and the 4,800/4,801-frame captures. These are historical records. No build, test, deployment or target playback ran in 4A. The current sandbox denied WSL startup, so 4B host checks require an available Linux execution session. [Phase 3 artifact and host results](phase3-execution-debug.md#negotiated-period-chunk-experiment)

## 1. Requirement traceability

Assessments below describe implementation seen in source. Runtime observations from Phases 1–3 remain historical.

| Approved requirement | Current implementation | Existing coverage or evidence | Remaining gap | Source assessment |
| --- | --- | --- | --- | --- |
| R1 / D1–D2. C++17 CLI plays one local WAV through the requested PCM, with WM8960 as the MVP target | [CLI `main`](../../../software/src/audio/play_wav_main.cpp#L6) passes the supplied path and PCM to [`play_wav_file`](../../../software/src/audio/wav_playback.cpp#L112). libsndfile decodes the file and ALSA opens the named PCM in blocking playback mode. [CMake target](../../../CMakeLists.txt#L28) requires C++17. | Host tests exercise `null`, file and unknown PCMs. Phase 1 used `aplay` for the WM8960 baseline. Phase 3 used F001 on WM8960 for the original 48 kHz mono clip. | Complete the WM8960 format matrix and successive opens. | Implemented |
| R2 / D3. Accept nonempty little-endian RIFF/WAVE PCM16, mono/stereo at 44.1/48 kHz. Validate before PCM open. | [`validate_wav_format`](../../../software/src/audio/wav_playback.cpp#L30) checks WAV/WAVEX container, endian, subtype, channels, rate and positive frame count. [`play_wav_file`](../../../software/src/audio/wav_playback.cpp#L124) calls it before `snd_pcm_open`. | [Host positives](../../../tests/audio_playback_test.py#L42) cover all four combinations with `null` and PCM16 extensible input with a non-WAV suffix. [Host negatives](../../../tests/audio_playback_test.py#L93) exercise validation before an invalid PCM. | Byte-level coverage is limited to 48 kHz stereo. libsndfile metadata normalization limits strict RIFF integrity checks, as documented in [Phase 1 design step 3](phase1-requirement-design.md#3-technical-design). | Implemented |
| R3 and approved final-chunk decision. Foreground playback once, one fixed-size buffer, submit all frames then drain. | [`transfer_frames`](../../../software/src/audio/wav_playback.cpp#L68) allocates one negotiated-period buffer. [`write_frames`](../../../software/src/audio/wav_playback.cpp#L50) advances by frames written times channel count. [`play_wav_file`](../../../software/src/audio/wav_playback.cpp#L146) obtains the period and drains after transfer succeeds. | [Host byte tests](../../../tests/audio_playback_test.py#L60) cover one-frame, aligned and multichunk partial 48 kHz stereo. Phase 3 reports 1,055 silent frames and a clean ending for the original mono fixture. | Reuse existing boundary checks and renew target listening evidence. Independent period measurement, a one-frame pad fixture and forced partial writes are deferred. | Implemented |
| R4. Diagnostic and nonzero exit for invalid arguments, file, decoder or ALSA failures. Release resources and use the selected PCM. | [CLI exit mapping](../../../software/src/audio/play_wav_main.cpp#L8), [scoped handles](../../../software/src/audio/wav_playback.cpp#L17), [input/open/configure failures](../../../software/src/audio/wav_playback.cpp#L118), [read failures](../../../software/src/audio/wav_playback.cpp#L85), [write failures](../../../software/src/audio/wav_playback.cpp#L53) and [transfer/drain cleanup](../../../software/src/audio/wav_playback.cpp#L146). | [Host negatives](../../../tests/audio_playback_test.py#L93) assert usage and file/device errors, including metadata precedence. | Runtime coverage for decoder and late ALSA failures and cleanup is deferred as residual risk. | Implemented |
| R5. Audio is opt-in. Default M0 builds and smokes without audio development dependencies. | [CMake](../../../CMakeLists.txt#L5) defines M0 and its smoke test before the default-OFF option. ALSA/libsndfile discovery and native audio test registration are [inside the option](../../../CMakeLists.txt#L18). [M0 entry point](../../../software/src/main.cpp#L3) retains its startup message. | [Phase 2](phase2-implementation-build.md#4-tests) records OFF 1/1 before audio development packages were installed. Phase 3 records ON 3/3. | Fresh OFF and ON build evidence for the reviewed baseline. | Implemented |
| D4. Software verification, independent WM8960 baseline and separate application listening acceptance | [CTest registration](../../../CMakeLists.txt#L39) and the [host fixture suite](../../../tests/audio_playback_test.py) exercise the CLI. The [Phase 1 preflight](phase1-requirement-design.md#hardware-preflight) is independent of the application. | **ENGINEER_PROVIDED, historical.** The Engineer heard the `aplay` baseline and confirmed a clean ending on the approved player for the original 48 kHz mono fixture. The baseline wrapper missed the exact `aplay` exit status. [Approved target retest](phase3-execution-debug.md#negotiated-period-chunk-experiment) | Application results and fresh listening confirmation for all four channel/rate combinations are pending. | Acceptance evidence partial |

## 2. Design & code review

**VERIFIED by inspection.** The CLI owns argument checks and reporting. The playback API keeps libsndfile and ALSA handles private, and the current [PlantUML flow](phase2-implementation-design.puml) agrees with validation, configuration, transfer, drain and cleanup. The PCM receives native-endian interleaved S16 at the source channel count and rate. The selected ALSA route can adapt those parameters to the hardware. The source contains no hard-coded WM8960 route or fallback PCM. [CLI](../../../software/src/audio/play_wav_main.cpp#L6) · [Playback API](../../../software/src/audio/wav_playback.hpp#L8) · [PCM setup](../../../software/src/audio/wav_playback.cpp#L133)

**Final-chunk logic.** For validated frame count `N > 0` and negotiated period `P > 0`, a successful read returns `min(unread, P)`. A short successful chunk can therefore occur only on the final iteration. Zero-fill starts at `decoded × channels`, so it preserves every decoded sample and clears any samples left over from the previous iteration. The loop submits one period, decrements by decoded frames and ends with exactly `(P - N % P) % P` padding frames. Aligned input adds zero frames. A decoder error or unexpected short read returns before padding or writing that chunk. The period query, zero-period guard and allocation bounds keep the buffer sized independently of clip length. [Approved decision](phase1-requirement-design.md#approved-final-chunk-decision) · [Transfer and write loops](../../../software/src/audio/wav_playback.cpp#L50) · [Period and drain](../../../software/src/audio/wav_playback.cpp#L146)

**Error handling and scope.** Positive partial writes advance the interleaved pointer and reduce the remaining frame count. Zero progress and negative writes, including underruns, return diagnostics as approved. Transfer or drain failure attempts a PCM drop and both scoped handles close. No finding observed in these paths, format ordering, module boundaries or build isolation on inspection. Late-error behavior has source-review evidence only. Mixer controls, automatic underrun recovery, other codecs and streaming TTS are outside the approved MVP. The original audible defect's underlying cause remains **UNKNOWN**. The prior listening result establishes the mitigation on one clip and route. [Approved scope and design](phase1-requirement-design.md#1-feature-understanding) · [Cleanup](../../../software/src/audio/wav_playback.cpp#L156) · [Phase 3 incident](phase3-execution-debug.md#l1--clipped-or-stuttered-clip-ending)

### Findings

| ID and severity | Evidence and logical failure mode | Impact and MVP disposition |
| --- | --- | --- |
| F001-4A-01 · **Minor** · Test coverage | `tests/audio_playback_test.py:verify_valid:60–89` derives its expected chunk size from the player's one-frame capture. If a regression consistently chunks and pads at half the actual ALSA period for an even period, the same output-derived value sizes both subsequent fixtures and their expectations. Those byte checks can still pass. **VERIFIED** test structure. The false pass is **INFERRED** from the oracle. [One-frame and byte checks](../../../tests/audio_playback_test.py#L60) | R3 and the approved period-padding decision lack an independent period oracle. MVP-03 retains the existing byte checks. Independent period measurement remains a residual risk. |
| F001-4A-02 · **Minor** · Test coverage | `tests/audio_playback_test.py:verify_invalid:93–123` covers usage, file validation and PCM open. No test forces `write_frames` to take a positive partial-write branch or forces `play_wav_file` to drop after a read/write/drain error. A future stereo offset error or missing drop could escape the suite. **VERIFIED** coverage gap. [Existing cases](../../../tests/audio_playback_test.py#L93) · [`wav_playback.cpp:write_frames:54`](../../../software/src/audio/wav_playback.cpp#L54) · [`wav_playback.cpp:play_wav_file:156`](../../../software/src/audio/wav_playback.cpp#L156) | R3 sample preservation under partial writes and R4 failure cleanup remain source-assessed. Fault injection and simulated stream failures are deferred as residual risks. |

No Critical or Major production defect was identified by static inspection. The WM8960 format matrix and listening confirmation remain required for D4 acceptance.

## 3. Proposed verification test plan

The three core groups below supersede the original T01–T09 proposal. Run the existing host suite once and use its byte-capture results for MVP-03. Use four short clips in the hardware group to cover the approved rate/channel combinations, with the original Phase 3 clip serving as the 48 kHz mono case. The table preserves the 4A planning status. [Section 5 records execution results](#5-4b-execution-evidence).

| Group | Priority / mode | Requirement or risk | Check and acceptance evidence | Status |
| --- | --- | --- | --- | --- |
| **MVP-01 · Build and existing software regression** | Core · Linux host | R1–R5 / D4 | Build the default audio-OFF configuration and require the existing M0 smoke test to pass (1/1). Confirm the M0 startup message and that the audio target is absent. Use an existing dependency-free environment if available. Otherwise disable ALSA and pkg-config discovery in the fresh OFF configure and retain the Phase 2 dependency-free run as historical evidence. Build with audio ON and run the existing CTest suite (3/3), including the four `null` format cases, file-sink byte checks and existing usage, WAV and PCM negatives. Record any skipped subcase. [Existing host tests](../../../tests/audio_playback_test.py) · [Phase 2 build and test evidence](phase2-implementation-build.md#4-tests) | NOT RUN |
| **MVP-02 · Raspberry Pi + WM8960 listening** | Core · Target + Engineer | R1–R4 / D4 and Phase 3 clipping (L1) | Use the matching Phase 3 ARM artifact and verify the Pi copy by SHA-256. Rebuild via the existing ARM workflow only if the artifact is unavailable or its provenance cannot be confirmed. Play the installed `Front_Center.wav` (48 kHz mono) plus one representative PCM16 clip each for 44.1 kHz mono, 44.1 kHz stereo and 48 kHz stereo through `plughw:CARD=wm8960soundcard,DEV=0`. Use recognizable end markers and run the clips successively to check reopen. Record artifact identity, command, PCM, fixture metadata and exit status. Require exit 0 and fresh Engineer confirmation for each clip that playback was audible at the expected speed with an intact ending, including a clean final “-ter” on `Front_Center.wav`. [Approved application acceptance](phase1-requirement-design.md#5-verification--risks) · [Phase 3 retest](phase3-execution-debug.md#negotiated-period-chunk-experiment) | NOT RUN |
| **MVP-03 · Final-chunk boundaries** | Core · Linux host, same CTest run as MVP-01 | R3 and approved padding decision | Read the existing `audio_playback_valid` file-sink results. The one-frame clip must preserve its sample frame and add only silence. The aligned clip must match the source byte for byte with zero padding. The multichunk partial clip must preserve all source bytes in order and append exactly the expected silent frames for the test-inferred chunk size. This checks final-only padding and guards against the previous extra silent period within the existing oracle's limits. [Current boundary assertions](../../../tests/audio_playback_test.py#L60) | NOT RUN |

### Deferred coverage and residual risks

- **F001-4A-01.** The file-sink test infers its chunk size from player output. It can miss a consistent mismatch with the actual ALSA period. An independent period probe, extra boundary fixtures and broader byte-capture coverage are deferred. The host format checks and WM8960 listening cover the supported combinations at their respective levels.
- **F001-4A-02.** Forced partial writes, decoder and ALSA failure simulation, and drop/close tracing are deferred. Existing negatives cover bad inputs and PCM-open failure. Runtime behavior after a stream failure remains a residual risk assessed through source review. Revisit fault injection or a specialized harness if evidence identifies a concrete Critical or Major defect that requires it.

## 4. 4A Engineer decision gate

**4A — APPROVED.** On 2026-11-08 the Engineer approved MVP-01, MVP-02 and MVP-03 and authorized Phase 4B, including the scoped Pi playback. The approved plan defers the two Minor coverage risks above. No product deviations were accepted.

## 5. 4B execution evidence

**VERIFIED subject.** The 2026-11-08 run used `79ebd32e1cce29a31c66aeab66ee3dcacdff7db7`, the 4A-reviewed revision. The scoped diff against HEAD was empty for application source, CMake, existing tests and build/deploy tools. Fresh host builds ran in Ubuntu 22.04 WSL x86-64 with GCC 11.4.0, ALSA 1.2.6.1 and libsndfile 1.0.31. Results below are from this run.

Evidence links under `build-f001-phase4b/` point to local run artifacts in the ignored build directory.

### MVP-01 · Build and existing software regression — PASS

| Subcase | Expected and basis | Actual and comparison | Result |
| --- | --- | --- | --- |
| Default audio-OFF build | R5 and the [M0 entry point](../../../software/src/main.cpp#L3) require a successful M0-only build, a passing smoke test and `AI Robot: M0 foundation ready.` The audio target must be absent. | Configure/build exited 0, the option defaulted to OFF, target help contained `ai_robot` and no `ai_robot_play_wav`, and the sole CTest test passed with the exact message. The fresh configure disabled ALSA and PkgConfig discovery as approved. CMake reported both disable variables unused, consistent with the OFF branch never reaching discovery. [OFF log](../../../build-f001-phase4b/host-off.log) · [Target list](../../../build-f001-phase4b/off-targets.log) | PASS |
| Audio-ON build and valid cases | R1–R3 and D4 require the CLI to build and exit 0 for PCM16 mono/stereo at both 44.1 and 48 kHz on `null`. The [existing positive tests](../../../tests/audio_playback_test.py#L42) also require the byte checks detailed under MVP-03. M0 must still pass. | Both executables built. `ai_robot_smoke` and `audio_playback_valid` passed. Each of the four `null` invocations exited 0 with empty stdout and stderr as asserted. All three byte checks passed. [ON log](../../../build-f001-phase4b/host-on.log) | PASS |
| CLI, file and PCM negatives | R4 and the [existing negative assertions](../../../tests/audio_playback_test.py#L93) require usage exit 2, and playback exit 1 with the expected error stage and format reason where applicable. Invalid WAVs must be rejected before the unknown PCM is opened. | All 12 assertions passed. Three usage cases exited 2 with usage. An unknown PCM exited 1 with `open PCM`. Missing, directory, mode-000 and malformed paths exited 1 with `open WAV`. Empty, 24-bit, three-channel and 32 kHz inputs exited 1 with the matching `validate WAV` reason. The unreadable-file case ran and no subcase was skipped. The ON CTest summary was 3/3. [ON log](../../../build-f001-phase4b/host-on.log) | PASS |

Reproduce on Linux from the repository root using separate build trees.

```bash
cmake -S . -B build-f001-phase4b/off -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  -DCMAKE_DISABLE_FIND_PACKAGE_ALSA=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=TRUE
cmake --build build-f001-phase4b/off --parallel
cmake --build build-f001-phase4b/off --target help
ctest --test-dir build-f001-phase4b/off --verbose --output-on-failure --no-tests=error

cmake -S . -B build-f001-phase4b/on -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  -DAI_ROBOT_ENABLE_AUDIO_PLAYBACK=ON
cmake --build build-f001-phase4b/on --parallel
ctest --test-dir build-f001-phase4b/on --verbose --output-on-failure --no-tests=error
```

### MVP-02 · Raspberry Pi + WM8960 listening — PASS

The approved Pi reported `Linux armv7l` and `wm8960soundcard`. The existing Phase 3 ARM executable was reused. Its Pi copy and local artifact both matched SHA-256 `e60ebc2c06b0f52616ebb0405ced47e9467c1e050ab52ea7d2a37218c332dbc4`. The installed `Front_Center.wav` matched its Phase 3 SHA-256. The three generated fixtures matched their local SHA-256 values on the Pi. The initial key-only SSH attempt was rejected, and the authenticated retry succeeded before playback. [Pi preflight](../../../build-f001-phase4b/pi-preflight.log) · [Fixture metadata and hashes](../../../build-f001-phase4b/listening-fixtures.log) · [Playback log](../../../build-f001-phase4b/pi-listening-Gx7thR6g)

D4 requires exit 0, audible output at the expected pace and an intact ending on the configured WM8960 PCM. The historical spoken fixture and generated note sequences supply the expected content and ending. All four commands used `--device plughw:CARD=wm8960soundcard,DEV=0` and ran successively, exercising reopen. No application diagnostics appeared. The Engineer gave fresh confirmation for all four clips in the listed order: “Yes, all four sounded as expected.”

| Exact fixture | Expected audible result | Actual process and Engineer result | Result |
| --- | --- | --- | --- |
| `Front_Center.wav` · 48 kHz mono PCM16 · 68,545 frames | “Front Center” at normal speech pace with a clean final “-ter” (L1 regression) | Exit 0. Engineer **YES** for audibility, pace and ending. | PASS |
| `mono-44100.wav` · 44.1 kHz mono PCM16 · 52,920 frames | Three rising notes over 1.2 s with an intact high final note | Exit 0. Engineer **YES** for audibility, pace and ending. | PASS |
| `stereo-44100.wav` · 44.1 kHz stereo PCM16 · 52,920 frames | Three notes over 1.2 s with an intact high final note | Exit 0. Engineer **YES** for audibility, pace and ending. | PASS |
| `stereo-48000.wav` · 48 kHz stereo PCM16 · 57,600 frames | Three notes over 1.2 s with an intact low final note | Exit 0. Engineer **YES** for audibility, pace and ending. | PASS |

The stereo fixtures carry distinct channel samples and keep every note audible on either channel. Listening verified the audible sequence and ending on the configured speaker. Stereo separation was outside this check.

The only new scripts are temporary verification artifacts in the ignored build directory. The [fixture generator](../../../build-f001-phase4b/generate-listening-wavs.py) writes the three supplemental WAVs. The [playback wrapper](../../../build-f001-phase4b/run-pi-listening.sh) checks the artifact and original WAV, transfers the generated fixtures into a fresh directory under the Pi user's home, and logs each invocation. Reproduce with `python3 build-f001-phase4b/generate-listening-wavs.py` followed by `bash build-f001-phase4b/run-pi-listening.sh` when the Engineer is ready to listen.

### MVP-03 · Final-chunk boundaries — PASS within the approved scope

These results come from the same `audio_playback_valid` run as MVP-01. The generated input PCM bytes are the independent sample reference. The existing test derives chunk size from the one-frame capture, which yielded an inferred size of 2,400 frames. Its expected partial padding follows the approved formula `(P - N % P) % P` for that inferred size. [Boundary assertions](../../../tests/audio_playback_test.py#L60) · [CTest log](../../../build-f001-phase4b/on/Testing/Temporary/LastTest.log)

| Fixture | Expected | Actual and comparison | Result |
| --- | --- | --- | --- |
| One-frame PCM16 stereo | The known source frame first, followed only by zeros, with complete stereo frames | The prefix and all-zero tail assertions passed. The capture established an inferred 2,400-frame chunk, so the tail comprised 2,399 frames. | PASS |
| Aligned PCM16 stereo | Exact source bytes for `2P` frames with zero added frames | Captured exactly 4,800 source frames in order with no padding. | PASS |
| Partial-final PCM16 stereo WAVEX | Exact source bytes for `2P + 1` frames, followed by `P - 1` zero frames | Captured 4,801 source frames byte for byte, followed by exactly 2,399 silent frames. The byte comparison excludes inserted silence between source chunks. | PASS |

F001-4A-01 remains a residual risk because the actual negotiated period was not independently measured. F001-4A-02 remains a residual risk because forced partial writes and late-error cleanup were source-reviewed and deferred from runtime verification.

## 6. Final assessment & handoff

**4B — APPROVED. Product verification — PASS.** The Engineer accepted Phase 4B on 2026-11-08. All three approved groups met their acceptance criteria, including a clean ending for the original `Front_Center.wav` on the approved WM8960 route. The two Minor coverage risks remain deferred under the approved plan.

**Next action.** Await separate Engineer authorization for Phase 5.
