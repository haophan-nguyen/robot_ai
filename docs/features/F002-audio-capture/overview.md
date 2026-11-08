# F002 — Audio Capture

F002 adds the microphone input path for the Voice Command Robot. The Audio subsystem will capture one five-second recording on the Raspberry Pi through its WM8960 and save a mono 48 kHz PCM16 WAV. F001 will play the recording for Engineer acceptance. The capture loop gives later speech recognition work a place to consume PCM frames.

**Phase 2 — Implementation & Build** is **APPROVED / PASS**. The Engineer approved the implementation, host checks, ARM build, and documentation.

| Phase | Status | Documentation |
| --- | --- | --- |
| 1 — Requirement & Design | APPROVED / PASS | [Phase 1 report](phase1-requirement-design.md) |
| 2 — Implementation & Build | APPROVED / PASS | [Phase 2 report](phase2-implementation-build.md) · [Implementation diagram](phase2-implementation-design.puml) |
| 3 — Execution & Debug | Authorized | — |
| 4 — Independent Verification & Review | Pending | — |
| 5 — Technical Handover & Engineering Mastery | Pending | — |

Next Engineer action. Participate in the authorized Phase 3 microphone and listening check.
