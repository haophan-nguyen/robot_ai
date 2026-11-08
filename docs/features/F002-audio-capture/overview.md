# F002 — Audio Capture

F002 adds the microphone input path for the Voice Command Robot. The Audio subsystem will capture one five-second recording on the Raspberry Pi through its WM8960 and save a mono 48 kHz PCM16 WAV. F001 will play the recording for Engineer acceptance. The capture loop gives later speech recognition work a place to consume PCM frames.

**Phase 1 — Requirement & Design** is **APPROVED / PASS**. The WM8960 preflight passed with Engineer listening confirmation, and the Engineer gave final Phase 1 approval on 2026-11-08.

| Phase | Status | Documentation |
| --- | --- | --- |
| 1 — Requirement & Design | APPROVED / PASS | [Phase 1 report](phase1-requirement-design.md) |
| 2 — Implementation & Build | Pending Engineer authorization | — |
| 3 — Execution & Debug | Pending | — |
| 4 — Independent Verification & Review | Pending | — |
| 5 — Technical Handover & Engineering Mastery | Pending | — |

Next Engineer action. Authorize Phase 2 when ready.
