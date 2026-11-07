# Phase 2 — Implementation & Build (V2.1)

## Purpose and boundary

Implement **only the Engineer-approved feature scope** and demonstrate reproducible compilation, suitable software tests, and an ARM target artifact. Phase 1 approval is a prerequisite; the Engineer must separately authorize starting Phase 2. **Build and authorized binary-only deployment are permitted. Running the feature on Raspberry Pi and diagnosing hardware behavior belong to Phase 3 and require separate approval.**

Read the feature `docs/features/<feature-id>-<short-slug>/overview.md`, its approved `phase1-requirement-design.md`, current source/CMake and existing build/deploy scripts. For legacy combined reports, use their approved Phase 1 section until migrated. Do not reopen settled design decisions without a concrete implementation blocker or discrepancy; explain and ask the Engineer before changing the approved contract.

## Implementation rules — practical Embedded C++17

1. **Follow the approved design and existing repository conventions.** Use C++17. Preserve M0/default build and unrelated source behavior. Prefer minimal, focused diffs.
2. **Keep responsibilities separate.** CLI handles arguments and diagnostics; reusable module implements playback; ALSA/libsndfile handles remain implementation details. Do not build a new framework for future TTS/capture/motion.
3. **Naming:** Follow established local conventions; otherwise prefer `snake_case` for functions/variables/files and `PascalCase` for types. Be consistent in each touched module. No forced project-wide renaming.
4. **Resource safety:** Use RAII or equally reliable scoped ownership for files, `SNDFILE*`, and ALSA PCM handles. Always clean up on success and error. Avoid raw owning pointers when a small wrapper suffices.
5. **Error handling:** Validate arguments and WAV metadata before opening the PCM; check every relevant read/configure/write/drain result. Report stage-specific errors and preserve useful exit codes. Handle partial PCM writes and bounded buffering without silent data loss.
6. **Configuration and portability:** Do not hard-code ALSA card index/PCM, absolute WSL paths, sysroot paths, or credentials into application code. Keep F001's approved `--device` boundary.
7. **Complexity discipline:** Prefer short, understandable functions, `const` correctness, fixed-size/bounded buffers, named constants, and comments explaining *why* rather than restating code. Add abstractions/enums only when they clarify a real supported case.
8. **Review before handoff:** Inspect the diff for accidental scope growth, portability assumptions, missing error paths, and new dependencies. Document meaningful changes without creating lengthy boilerplate.

## Execution workflow

1. **Inspect:** Confirm Phase 1 is `APPROVED`; extract relevant requirements R1–R5, design decisions D1–D4, acceptance criteria, and source-file expectations. Locate the exact scripts and their CLI. **Do not invent a deploy script name.**
2. **Implement:** Add the minimal C++17 playback module and CLI. Keep audio dependencies opt-in in CMake, and preserve default M0 build. Add focused, reproducible WAV fixtures/tests and negative test cases as appropriate.
3. **Software checks (authorized on WSL):** Run the available host/native software checks, including `ALSA null` where possible, and check M0 smoke tests. An ARM cross-built executable cannot normally run directly on x86_64 WSL; use a separately configured native host build if needed. Distinguish tested from untested cases.
4. **ARM cross-build:** Use **`tools/scripts/build-armhf.sh`** as the primary target build entry point. Reuse the validated toolchain/sysroot from the development environment baseline. If the script only builds M0, adapt its reusable target/option handling without breaking the prior M0 workflow. Do not improvise a second parallel build process when the script can be extended cleanly.
5. **ELF verification:** Record the actual `file` output and key `readelf` header/attributes/interpreter/dynamic dependency evidence. Verify ARM ELF32 hard-float compatibility with the Raspbian armhf target, and confirm required linked libraries correspond to the selected sysroot. Compilation/link success is **not** evidence that target execution works.
6. **Binary-only deployment:** If the Engineer's Phase 2 authorization includes deployment, use the **existing deploy script discovered in `tools/scripts/`**, with an explicit artifact and Pi destination under the user's home directory. Keep previous deployment targets safe, do not install packages or modify Pi system configuration, and do not execute the F001 binary on the Pi. Record path and transfer/digest evidence when available. If the existing script does not support the new executable, make a minimal reusable improvement and document it.
7. **PlantUML design:** After implementation, create **`phase2-implementation-design.puml`** beside Phase 2 documentation. Depict the *implemented* test app/modules, important call relationships and data flow (e.g. CLI → WAV validation/reader → PCM transfer → ALSA), plus important error paths. Where readable, show the key application-defined function names; do not turn the diagram into a full symbol dump. Add a relative link in the report. A rendered image is optional; do not claim rendering unless checked.
8. **Function Inventory:** From the actual implemented source, document the important **application-defined functions/methods** and public entry points. For each, provide signature (or relevant name and parameters), source path, responsibility, important callees or dependencies, and notable failure/return behavior. Clearly mark public interfaces vs private helpers. Cover the CLI entry, validation, WAV reading/PCM transfer, and resource cleanup as applicable. Do not enumerate standard-library or ALSA/libsndfile APIs as if they were project functions, and do not paste entire implementations. Keep the inventory proportional to the feature.
9. **Report and stop:** Write the Phase 2 report to **`docs/features/<feature-id>-<short-slug>/phase2-implementation-build.md`**, not under the Phase 1 report. Update `overview.md` with the report/diagram links and Phase 2 status; do not duplicate the report in the overview. Do not start Phase 3, play audio on the Pi, or claim application/hardware verification PASS without explicit Engineer authorization and evidence.

## Output contract — concise Phase 2 report

Create **`docs/features/<feature-id>-<short-slug>/phase2-implementation-build.md`** with title **`Phase 2 — Implementation & Build Report`**. This report is a separate Markdown artifact, linked from the feature `overview.md`. Do not append it to the Phase 1 file. Keep it readable and evidence-driven:

| Section | Required content |
|---|---|
| **1. Scope & implementation** | Feature ID, approved requirements covered, changed/new source paths, key implementation decisions or deviations |
| **2. Build** | Exact invocation of `tools/scripts/build-armhf.sh`, target/config, PASS/FAIL, executable path, compiler/sysroot identity |
| **3. ELF evidence** | `file` / `readelf` outputs or short excerpts showing ARM ELF32, EABI/hard-float, interpreter and library dependencies; clearly mark missing checks |
| **4. Tests** | Test name, host vs target environment, PASS/FAIL/BLOCKED, actual evidence and limits; M0 regression status |
| **5. Deploy** | Script used, binary/destination, transfer verification, PASS/FAIL/NOT RUN; **no target execution** |
| **6. Design artifact** | Relative link to `phase2-implementation-design.puml`, what the diagram captures, and rendering status (if checked) |
| **7. Function Inventory** | Table of important application functions/methods: signature/name, source file, purpose, important calls/dependencies, return/error behavior; distinguish public interfaces and helpers |
| **8. Issues & handoff** | Remaining risks, blockers, Engineer decisions, readiness status and one next action |

**Function Inventory example format (populate from actual implementation; do not invent symbols):**

| Function / signature | Source file | Purpose | Calls / dependencies | Errors / return |
|---|---|---|---|---|
| Actual project function | Relative file path | One-line responsibility | Main project/library calls | Success and failure behavior |

**Example compact status table (populate only actual results):**

| Check | Status | Evidence |
|---|---|---|
| ARM cross-build | PASS / FAIL / BLOCKED | Command + build output |
| ELF architecture/ABI | PASS / FAIL / BLOCKED | `file`, `readelf` |
| Host software tests | PASS / FAIL / BLOCKED / NOT RUN | CTest/ALSA null logs |
| M0 regression | PASS / FAIL / BLOCKED / NOT RUN | Existing smoke test |
| Binary deployment | PASS / FAIL / BLOCKED / NOT RUN | Script + destination + hash if available |
| Pi application execution | **NOT RUN (Phase 3)** | No claim of playback |

## Decision gate

At the end, ensure the independent Phase 2 report and updated feature overview are both present; report **`READY FOR REVIEW`**, **`NEEDS ENGINEER DECISION`**, or **`BLOCKED`**, and stop. Do not mark Phase 2 `APPROVED/PASS` without Engineer acceptance of the results. **Phase 3 requires a separate explicit Engineer approval**, even if cross-build, host tests, and binary transfer all pass. The Engineer leads debugging of substantive target failures; do not preempt that by running the F001 application in Phase 2.
