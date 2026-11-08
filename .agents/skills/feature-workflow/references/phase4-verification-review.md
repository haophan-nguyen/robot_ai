# Phase 4 — Independent Verification & Review (V2.5 · Risk-Based MVP Verification + Test Implementation)

## Purpose and entry criteria

Have a **separate, independent reviewer agent** evaluate whether the implemented feature meets its **approved requirements**, whether the design and code are sound for the MVP, and what evidence is still missing. Then execute **only Engineer-approved verification cases** and report actual results. This phase is not a new implementation sprint, an attempt to maximize discovered defects or test count, or an automatic approval of previous agents' work. **Optimize for confidence in the approved MVP at proportionate cost.**

**Prerequisites:** Engineer explicitly approves completion of Phase 3 and separately authorizes Phase 4. Review the **current feature and actual source revision**, including any fixes made during Phase 3 (for F001: the approved final-chunk padding experiment). If the code version to review is ambiguous, label the ambiguity and ask; do not silently review an old binary/commit. Phase 4 comprises **two mandatory checkpoints**:

- **4A — Independent Review & Test Planning:** inspect, evaluate, propose, and **STOP for Engineer approval**. No product-code edits or test execution.
- **4B — Verification Execution & Final Assessment:** run **only the approved test plan**, collect evidence, and **STOP for Engineer acceptance**. No Phase 5 without a new, explicit authorization.

## Reviewer independence and authority

- Use a **different agent/session from the Phase 2/3 implementer** with fresh review context. The Engineer initiates this separation. If separation cannot be established, disclose the limitation rather than claiming an independent review.
- Treat Phase 1–3 reports as **requirements, design intent, decisions, and historical evidence**, **not proofs** that the source is correct. Independently inspect actual code, tests, build configuration, and relevant changes. Prior PASS statements must not be copied into Phase 4 as fresh verification.
- The verifier owns **findings and test evidence**, not production fixes. It must **not alter application source, CMake/build scripts, existing tests, approved designs, Phase 1–3 reports, or target system configuration** without separate explicit Engineer instruction. A defect is assigned to an implementation owner; the verifier retests the resulting changed revision after Engineer approval. Never silently self-fix and self-certify.
- **4A write scope:** The verifier may create/update only `docs/features/<feature-id>-<short-slug>/phase4-verification-report.md` and the feature `overview.md`. No test creation, test execution or repository test changes in 4A. Preserve prior report/history and failed evidence.
- **4B write scope (after approved test plan):** In addition to the separate Phase 4 report and compact overview, the verifier may create **minimal new test scripts and fixtures** within the feature's `tests/` area, and temporary test outputs/logs under safe test/build locations. These are **verification artifacts**, not production changes. Existing tests, existing assertions/expected values, test registration/CMake and build/deploy scripts remain unchanged unless the Engineer separately authorizes an edit. Do not weaken tests to force a PASS.
- Running a test with destructive side effects, privileged access, package installation, volume/mixer changes, reboot, driver/device-tree changes, or broad remote operations **requires separate authorization**.
- Engineer owns review priorities, test-plan approval, hardware listening judgments, acceptance of deviations, and phase closure.

## Context to load — Phase 4 exception to progressive disclosure

**Phase 4 is a cross-phase audit.** Unlike Phases 1–3, it must inspect **all relevant feature materials**, then verify them against the current implementation:

1. Feature `overview.md`; **entire approved** `phase1-requirement-design.md`, `phase2-implementation-build.md`, `phase3-execution-debug.md` (including debug history, outcomes and remaining uncertainties).
2. `phase2-implementation-design.puml`, approved decision notes and relevant project/architecture documentation.
3. **Actual implementation** for this feature, CLI, CMake/options/build/deploy scripts as relevant, and the **actual existing test code and fixtures**; inspect current worktree status, revision identity and relevant diff.
4. Referenced logs or target evidence when a finding depends on them. Don't read unrelated codebases or all project documents without a direct reason.

Record any discrepancies between approved documentation and current code as findings; distinguish a justified, Engineer-approved Phase 3 change from an unapproved deviation. **The feature's final implementation is the object under review, not solely its Phase 2 snapshot.**

## 4A — Independent Review & Test Planning (inspection only)

### Risk-first review policy — mandatory

**Quality is measured by important risks covered and the clarity of the Engineer's decision, not by the number of findings, test cases, or lines of report.** Before deep review or proposing tests:

1. Identify the **core user-visible behavior** and approved acceptance criteria. For a simple MVP, concentrate on whether the feature works reliably for its intended use.
2. Identify **known regressions from Phase 3**, material correctness/safety risks, and requirements with no meaningful evidence. Read existing tests first to avoid retesting already well-covered scenarios.
3. Prioritize each candidate risk by **impact × likelihood**, informed by actual code/evidence; consider detection value and execution cost. Use simple labels `HIGH / MEDIUM / LOW` with a one-line rationale; no numerical scoring framework required.
4. Select the **minimum sufficient verification**. Reuse build/CTest, approved scripts and representative hardware checks. Prefer simple boundary tests when a real defect occurred; avoid injecting hypothetical failures or building elaborate shims merely to increase coverage.
5. Distinguish **blockers**, **meaningful but deferrable risks**, and **optional improvements**. A low-risk uncovered branch is a residual risk, *not* automatically a required new test or defect.

**Stop expanding review/tests when** approved core behavior, important known regressions and credible high-impact risks have an adequate verification path. Do not manufacture additional findings to fill a quota. Still surface any genuine Critical/Major issue encountered, even if outside the initial shortlist.

### A1. Requirement and acceptance traceability

Build a compact mapping **approved requirement / criterion → actual implementation path or function → existing test or execution evidence → coverage gap → assessment**.

- Evaluate requirements **individually**: implemented / partially implemented / missing / unclear. Do not label a requirement PASS merely because an earlier report says PASS.
- Compare **in-scope vs. out-of-scope** boundaries. Proposed future PCM streaming, TTS, and extra audio codecs are **not defects** in F001 WAV MVP unless an approved requirement demands them.
- If a requirement or expected behavior is ambiguous, cite the exact ambiguity and recommend a decision, rather than inventing new acceptance criteria.

### A2. Design vs. implementation review

Confirm implemented module boundaries, main data flow, public interface and CLI, error paths, PlantUML function/data flow, and documented Phase 3 decisions. Check whether the design is adequate for the approved feature, without demanding a future framework. Flag meaningful design drift and unverified assumptions.

### A3. Risk-based C++/ALSA code review

Inspect relevant code and tests in descending risk order; do not treat the following as a mandate to test every branch:

- **Correctness:** decoded PCM frame/sample/channel handling, mono/stereo correctness, 44.1/48 kHz input and negotiated ALSA parameters; frame counts, sample format, channel conversion behavior and possible integer arithmetic errors.
- **Final-chunk boundaries:** negotiated period/chunk size, handling of incomplete/aligned chunks, padding only at **end of file/stream**, no extra period without design approval, zero-frame/error cases, correct drain semantics; ensure real PCM frames are never overwritten or dropped.
- **I/O & robustness:** partial writes and recoverable ALSA errors, readable diagnostics and exit status, negative input validation, safe resource cleanup across all return paths, bounded memory, file read errors.
- **Maintainability:** clear responsibilities, limited coupling and sensible naming; avoid cosmetic/style findings unless they affect engineering quality.
- **Evidence limits:** distinguish source-inspection concerns from bugs reproduced by a test. Do not claim a hardware sound-quality check from source review or successful process exit.

### A4. Findings policy

Every actionable finding must include a **stable ID**, severity, `file:function:line` (where available), concrete evidence, reproduction/scenario or logical failure mode, impact on an approved requirement, and a proposed smallest remedy **or discriminating test**. Do not invent line numbers. **No finding is an acceptable review outcome.** Report substantive issues, not speculative defects or missing tests for their own sake; group related minor observations rather than inflating the count.

| Severity | Meaning | Gate guidance |
|---|---|---|
| **Critical** | Memory corruption, unsafe behavior, destructive failure, or major safety/integrity risk | Block product approval until resolved/explicitly risk-accepted |
| **Major** | Violates approved requirement, known functional failure, material error-path or reliability flaw | Normally blocks Phase 4 acceptance |
| **Minor** | Real maintainability or limited-impact defect | Engineer decides whether fix or defer |
| **Suggestion** | Optional improvement, performance idea, or future feature outside scope | Do not turn into mandatory MVP work |

Include a **No finding observed** statement for an area only when it was inspected, and clarify that absence of findings is not proof of absence of defects. Flag uncertain observations as `INFERRED` or `NEEDS TEST`, never `VERIFIED`.

### A5. Propose a minimum sufficient, risk-based test plan

Propose **about 3–5 core test groups for a simple MVP**, or fewer if the existing verification is already sufficient. This is a **guideline, not a quota or limit**: scale up only when complexity, a credible Critical/Major risk, or the Engineer's request justifies additional checks. An approved test group may reuse multiple existing test cases without counting each assertion as a separate new proposal.

For each selected group, provide the compact fields:

`Test group / ID | Risk & rationale (HIGH/MEDIUM/LOW) | Requirement | Test approach and expected result | Reused evidence/tests | Cost (LOW/MEDIUM/HIGH) | Status`

**Selection order:**

1. **Existing build + test regression:** Run established CTest/build paths before inventing tests; group existing validations as one verification activity where appropriate.
2. **Core end-to-end acceptance:** Demonstrate intended user-visible behavior on the target/actual environment. Audio quality requires Engineer listening confirmation; exit code alone does not establish audio quality.
3. **Known defect regression:** Recheck the exact failure seen in earlier phases and one or two representative boundary cases if the code path makes them meaningful.
4. **Material uncovered risk:** Add focused, deterministic negative/edge tests only if a credible failure would materially affect approved requirements and is not reasonably covered already.

**Default exclusions unless justified by a specific high-impact risk or Engineer request:** broad fault-injection matrices, process interposers or custom shims, exhaustive error/cleanup branch testing, full cross-product hardware combinations when host coverage exists and representative target checks are adequate, cosmetic lint/refactoring, and infrastructure built only to facilitate a hypothetical test. Such techniques may be valid for high-risk software but **must not be the default for a simple feature**.

**F001 Audio Playback — illustrative compact plan, not a mandatory suite:**

- **Core group 1 — Host regression:** Existing opt-in/default build and CTest; covers supported WAV format combinations, invalid input and known host checks where already implemented.
- **Core group 2 — Target audio acceptance:** Representative mono and stereo WAV playback on Raspberry Pi/WM8960, using different supported sample rates where practical; Engineer confirms audible quality. Reuse the approved deployment script and artifact identity check as steps, not stand-alone test projects.
- **Core group 3 — Final-chunk regression:** Exercise aligned/no-pad and partial-final-chunk behaviors, reusing existing byte/frame checks; repeat the original `Front_Center.wav` audible ending. Add one lightweight boundary case only if genuinely missing and important.

**Coverage caveat:** Representative target tests do not prove every supported format/device combination works on hardware. State any untested variants and fault paths as `DEFERRED / RESIDUAL RISK` with concise impact rationale; do not silently label them PASS. A Minor coverage gap does **not** block the MVP by itself. Preserve acceptance obligations for any genuinely required or high-risk behavior.

**Do not convert a risk inventory into a test inventory.** Present at most the priority tests needed for the current approval decision and a short deferred-risks summary. Avoid nested subcase explosions or tooling work disproportionate to the feature.

### A6. 4A output and STOP gate

Create/update the **independent Phase 4 report** (layout below). Populate reviewer identity/context, traceability, design/code findings and **proposed** test matrix. Mark every unexecuted case **`NOT RUN`**. Report `4A — READY FOR ENGINEER REVIEW` or `4A — NEEDS ENGINEER DECISION/BLOCKED`; list only the decisions needed now: accept/modify the minimal test groups, disposition of genuine blockers, explicit acceptance/deferral of residual risks, and permission to execute 4B. **Stop; no Phase 4B tests automatically.** Update `overview.md` with a short link/status only.

## 4B — Approved Verification Execution (test scripts allowed; no production fixes)

**Start only after the Engineer explicitly approves the 4A test plan and authorizes 4B.** Capture the approved test IDs, scope of local/target execution, and any required hardware authorization. If a change to test scope is needed, seek approval before execution.

### B0. Test-case implementation and oracle policy — mandatory

The Engineer approves **what to verify** in 4A; the independent verifier chooses **how to verify** in 4B within the approved scope. Implement test cases only when existing automation cannot provide the approved evidence.

- **Tooling freedom:** Python, C++, shell, CTest or another appropriate language/tool is permitted. Select the **simplest reproducible approach**; prefer current test utilities and scripts. Do not build a new framework, shim or dependency for a routine MVP check.
- **Persistent tests:** New test scripts and minimal generated fixtures may live under `tests/` for future regression. Give scripts descriptive names and short run instructions. Keep generated data small; do not commit binaries, heavy recordings, transient logs or credentials. Do not silently change existing tests, assertions, expected values, fixtures or production code; ask separately if unavoidable.
- **Clear test oracle:** For **every executed case or meaningful subcase**, record (a) input/setup and requirement, (b) expected behavior or numeric result, (c) **independent basis** for the expectation (approved criterion, known fixture metadata, formula or independently computed reference), (d) actual observed result, (e) objective PASS/FAIL comparison and reproducible evidence (command, log, output artifact or calculated values). A PASS based solely on the system under test reporting its own expected value is **circular**; independently validate it or disclose the limitation and narrow the claim.
- **Status semantics:** `PASS` only when observed evidence meets the defined oracle; `FAIL` when measured behavior contradicts it; `BLOCKED` when execution or essential confirmation cannot occur; `NOT RUN` when not attempted. `PENDING ENGINEER CONFIRMATION` is not audio-quality PASS. If a subcase is skipped, report it; do not silently count it as PASS.
- **Audio acceptance:** CLI exit `0` and absence of ALSA errors show process execution, **not sound quality**. For hardware listening, state the exact fixture, PCM/device, artifact/revision, what the Engineer must hear (normal speed, correct content/channels where assessable, clean ending) and request an explicit YES/NO. When feedback is not yet obtained, leave that subcase pending or blocked, not passed.
- **Minimality:** Design tests to demonstrate approved behavior and prior regressions. Group related cases, reuse established host tests and hardware checks, and do not grow the approved plan simply because a new tool can generate more cases.
- **Failures and independence:** A failing case must remain visible with inputs, observations and reproducible steps. Never edit implementation or tune expected values until it passes; provide findings to the Engineer and have the implementation owner fix it. The verifier may re-run affected tests against a newly approved revision.

**Compact per-test evidence template** (Markdown row or short subsection; avoid a large form):

`ID | Input/setup | Expected + oracle source | Actual | Evidence (command/output/Engineer feedback) | PASS/FAIL/BLOCKED/NOT RUN | Limitations`

1. **Identify the reviewed subject proportionately:** Record source revision, material worktree changes, build/target artifact identity and relevant target PCM. Record hashes, build ID and fixture metadata only where useful to establish what actually ran. Compare to the 4A reviewed revision. If source changed materially, note the delta and reassess affected findings/tests before claiming verification.
2. **Implement only necessary test cases, then execute the approved plan:** First reuse existing CTest/build/deploy and test code. If an approved case lacks a practical test, create a lightweight test script or fixture under `tests/` using a suitable language; avoid unrelated test infrastructure. Keep new scripts readable and reusable, and include a command to rerun them. Preserve unrelated files. Run software tests on the correct host environment and hardware tests on the approved Pi only as allowed. Distinguish new executions from previously cited Phase 2/3 results.
3. **Collect oracle-backed evidence:** For each case/subcase capture input, expected result **and how it was determined**, actual observed behavior, comparison, command/log/artifact reference and `PASS`, `FAIL`, `BLOCKED` or `NOT RUN`. Measured frames/PCM bytes/error codes must be compared with a justified expected result, not a circular test-output-derived expectation. Audio quality requires **new Engineer YES/NO** for the exact clip on the reviewed hardware; exit `0` alone is not sufficient.
4. **Handle failures independently:** Log evidence and identify the associated finding/requirement. Do not modify product code or quietly adjust tests/expectations to pass. Give the Engineer a recommendation; request fixes from the implementation owner. If an approved fix arrives, record its revision and rerun impacted verification and sensible regression cases, with separate approval where needed.
5. **Assess product readiness based on material risk:** Summarize verified core requirements, known regressions, significant defects, explicitly deferred risks and relevant hardware/coverage limits. Do not block for Minor theoretical gaps that the Engineer knowingly accepts; do not bury Critical/Major risks to meet a small test budget. State `PASS`, `FAIL`, or `BLOCKED` for the **product** on the evidence available. `PASS` requires Engineer agreement and resolution/explicit acceptance of blocking issues. Product PASS is not Phase 5 mastery PASS.
6. **Report and stop:** Update `phase4-verification-report.md` and compact `overview.md`, and retain only newly necessary 4B test scripts/fixtures under `tests/` as authorized above. List any new test paths and how to rerun them. End as `READY FOR REVIEW`, `NEEDS ENGINEER DECISION`, or `BLOCKED`; do not start Phase 5 without distinct Engineer approval.

## Required output — ONE separate Phase 4 report

Write **`docs/features/<feature-id>-<short-slug>/phase4-verification-report.md`**, with title **`Phase 4 — Independent Verification & Review Report`**. This is **not** an appendage to Phase 1, 2 or 3, and the `overview.md` must **link to the separate report** using a relative Markdown link; it must not duplicate its findings, test plan or logs.

Use these sections (scale tables to feature size):

| Section | Required content |
|---|---|
| **0. Review scope & independence** | Feature/revision/worktree, reviewer session independence, which source/docs/tests actually inspected, unreviewed limits |
| **1. Requirement traceability** | Requirement → source path/function → existing proof → gap → assessment; separate current evidence from past claims |
| **2. Design & code review** | Design alignment, risk-based code analysis, **ID/severity/evidence/path/impact/recommendation** for each finding |
| **3. Proposed verification test plan** | **Small set of risk-ranked groups**, expected result, reused tests, estimated effort, rationale for any new tests, planned status (`NOT RUN`), and concise deferred-risk list |
| **4. 4A Engineer decision gate** | Proposal summary, unresolved findings, explicit Engineer-approved test IDs/deviations and approval record; **not approved yet** if absent |
| **5. 4B execution evidence** | Test script/path and rerun command where new; input/setup, **expected + independent oracle source**, actual observation, PASS/FAIL/BLOCKED/NOT RUN and evidence per approved case; Engineer listening confirmation and source revision |
| **6. Final assessment & handoff** | Product-level PASS/FAIL/BLOCKED, coverage gaps, defects, approved deviations, 4B review status, one next Engineer decision |

At 4A, sections 5–6 must clearly say **`NOT RUN / PENDING APPROVAL`**; never present a complete verification report based only on a plan. At 4B, preserve the 4A baseline and append evidence, test-plan approvals and any changed-code retest details. Do not claim another agent's PASS as independently reproduced.

## Exit criteria / Engineer gates

- **4A complete:** Relevant prior phase docs **and actual current code/tests** independently reviewed; requirements mapped; findings are evidence-based; **minimum sufficient, risk-ranked test groups** proposed; deferred low-risk coverage identified; Engineer decides what is in/out and whether 4B is authorized.
- **4B ready for acceptance:** Approved tests implemented only where needed and executed or explicitly blocked/not run; each test has justified oracle, expected vs actual and reproducible evidence; failures and residual risks stated; Engineer hearing confirmations obtained when required; product readiness assessed without inventing evidence.
- **Phase 4 accepted:** Engineer explicitly approves the final product verification result or knowingly accepts documented deviations. Only then can a **separate approval** begin Phase 5.

If the Engineer requests **skill maintenance only**, edit the Skill/reference files and stop. Do not inspect/review/execute F001 or create its Phase 4 report until separately instructed to begin the phase.
