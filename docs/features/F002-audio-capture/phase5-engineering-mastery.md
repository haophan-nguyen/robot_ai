# F002 — Phase 5 Engineering Mastery

**READY FOR ENGINEER REVIEW · 2026-11-08**

## Subject and source inventory

**F002 Audio Capture.** The Engineer confirmed Phase 4 approval and authorized Phase 5. **VERIFIED by inspection.** The source, CMake, tests and helper script contents match the approved correction commit `dd5f8940aaf256d16fde168101b2664f988e8d7c`. The recorder CLI matches the exact file reviewed in the independent G03 retest, and the local corrected ARM artifact matches the saved digest. [Phase 4 approval](phase4-verification-report.md#6-final-assessment--handoff) · [Retest identity record](../../../build-f002-phase4b/host/identity-sigxfsz-retest.log)

- **Design and phase records.** Reviewed the overview, all four reports, the sequence diagram, subsystem context, ARM workflow and F001 compatibility evidence. [Overview](overview.md) · [Phase 1](phase1-requirement-design.md) · [Phase 2](phase2-implementation-build.md) · [Sequence diagram](phase2-implementation-design.puml) · [Phase 3](phase3-execution-debug.md) · [Phase 4](phase4-verification-report.md) · [Subsystem context](../../sessions/2026/2026-11-03.md) · [ARM workflow](../../project/arm-cross-compilation.md) · [F001 verification](../F001-audio-playback/phase4-verification-report.md)
- **Implementation and tools.** Reviewed the complete recorder CLI, capture API and implementation, F001 playback integration, both audio tests, M0 and CMake entry points, ARM toolchain and build, test and deploy helpers. [Recorder CLI](../../../software/src/audio/record_wav_main.cpp) · [Capture API](../../../software/src/audio/wav_capture.hpp) · [Capture source](../../../software/src/audio/wav_capture.cpp) · [F001 playback](../../../software/src/audio/wav_playback.cpp) · [Capture test](../../../tests/audio_capture_test.py) · [Playback tests](../../../tests/audio_playback_test.py) · [CMake](../../../CMakeLists.txt) · [Toolchain](../../../tools/cmake/arm-linux-gnueabihf.cmake) · [Scripts](../../../tools/scripts/)
- **HISTORICAL evidence.** Inspected the saved host build and test outputs, ARM inspection logs, Pi transcripts, G02 check and original and corrected G03 evidence. The archived evidence referenced here was available in this workspace. [Phase 2 host results](phase2-implementation-build.md#4-tests) · [Phase 3 transcript](../../../build-f002-phase3/target-evidence.log) · [Phase 4 execution evidence](phase4-verification-report.md#5-4b-execution-evidence)

## Documentation synthesis

The handover connects the microphone route, frame budget, WAV readback and atomic publication to their source locations. It gives an incoming Engineer reproducible host build and Pi deployment commands, an independent WAV check, the listening criteria and maintenance entry points. The root README describes the M0 snapshot. The F002 handover is the capture guide. [Technical handover](technical-handover.md) · [Root README](../../../README.md)

**Evidence boundary.** G02 listening and WAV integrity apply to the original ARM recorder. The corrected CLI has independent host regression and file-size-limit evidence and an ARM build with static ABI inspection. Pi runtime evidence for that binary remains pending. **DEFERRED.** Hardware read edge cases, late publication races and finalization faults remain Phase 4 coverage limits. [Phase 4 execution and retest](phase4-verification-report.md#5-4b-execution-evidence) · [Residual limits](phase4-verification-report.md#6-final-assessment--handoff)

## Five-question Engineer knowledge check

Answer in your own words.

1. How do the recorder CLI, ALSA, libsndfile and F001 divide responsibility in the microphone-to-replay path?
2. How does `record_wav_file` move a recording from staging to publication, and what happens if another process creates the destination during capture?
3. If 1,000 frames remain and the negotiated period is 1,024 frames, how does the loop determine the next request after a read of 600 frames, and what changes if only 599 are written?
4. Why did the original G03 run terminate the recorder and leave a staged file, and who owns the signal policy when a program calls `record_wav_file` directly?
5. How would you add a caller-selected duration while keeping five seconds as the default, and what host and Pi evidence would you require to accept the change?

## Engineer answers and feedback

| Question | Status |
| --- | --- |
| Q1 | PENDING ENGINEER RESPONSE |
| Q2 | PENDING ENGINEER RESPONSE |
| Q3 | PENDING ENGINEER RESPONSE |
| Q4 | PENDING ENGINEER RESPONSE |
| Q5 | PENDING ENGINEER RESPONSE |

## Readiness and approval

The handover is ready for Engineer review. Phase 4 product verification is **APPROVED / PASS**. Phase 5 mastery and any learning deferrals are pending review of the Engineer's answers. F002 remains open until the Engineer explicitly accepts the handover and mastery and approves closure.

**Next Engineer action.** Answer Q1–Q5 in your own words.
