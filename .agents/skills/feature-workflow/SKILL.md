---
name: feature-workflow
description: Use for one-feature-at-a-time work in the AI Robot repository, from requirements/design through implementation, target debugging, verification, documentation, and engineering mastery. Follow the five-phase lifecycle with engineer-controlled handoffs and phase-specific references. Do not execute feature work when asked only to maintain this skill.
---

# AI Robot — Feature Workflow (V2.5, 5 phases)

## Purpose

Build **one feature at a time** while achieving three outcomes: **working product, trustworthy documentation, and engineer-owned technical understanding**. The agent investigates, automates repetitive work, proposes solutions, and collects evidence. The engineer owns requirements, consequential decisions, meaningful debugging, and verification of personal understanding.

## Lifecycle and responsibilities

| Phase | Required outcome | Agent / engineer boundary |
|---|---|---|
| **1 — Requirement & Design** | Approved feature scope, proposed design, acceptance criteria, identified unknowns and risks | Agent investigates and recommends; engineer decides and approves design |
| **2 — Implementation & Build** | Code implementing the approved scope, focused review, build and suitable unit-test results | Agent may implement within authorization; engineer reviews important choices and changes |
| **3 — Execution & Debug** | Target execution evidence and investigated failures | Agent runs permitted checks and collects evidence; **engineer leads hypotheses and experiments for meaningful technical faults** |
| **4 — Independent Verification & Review** | Independent code/design/requirement audit, Engineer-approved test plan, and evidence-based verification | **Separate reviewer agent** performs risk-prioritized 4A review/test proposal, waits for Engineer approval, then 4B may implement minimal test scripts and execute approved tests with explicit PASS/FAIL oracles; **no self-fixing product code** |
| **5 — Documentation & Engineering Mastery** | Updated durable docs, code/data-flow walkthrough, knowledge-gap assessment, engineer self-explanation and feature closure | Agent explains, asks questions, and drafts notes; **only engineer may confirm their own learning** |

**Every feature passes through all five phases**, but depth and rigor scale with complexity and risk. Maintain one short feature overview plus a separate report per phase; create phase reports on demand, not all at once. A small feature may complete a phase with a short report. No phase is silently skipped. **The engineer explicitly authorizes each transition**, including the start of Phase 2 and closure after Phase 5. A report marked READY is not approval.

Documentation is **continuous**, not deferred to Phase 5: Phase 1 saves design; Phase 2 notes significant implementation choices; Phase 3 logs meaningful debug evidence; Phase 4 records verification. Phase 5 consolidates and checks coherence, removes duplication, and records knowledge. Do not close a feature solely because Phase 4 tests pass.

## Feature document layout — mandatory

For each feature, use a **feature-level overview** and **separate, phase-scoped Markdown reports**. Do not append Phase 2–5 reports into the Phase 1 report, and do not use one growing catch-all `.md` as the only feature report.

```text
docs/features/<feature-id>-<short-slug>/
├── overview.md                        # Small index and current status
├── phase1-requirement-design.md       # Created in Phase 1
├── phase2-implementation-build.md     # Created in Phase 2
├── phase2-implementation-design.puml  # Created in Phase 2
├── phase3-execution-debug.md          # When Phase 3 begins
├── phase4-verification-report.md      # When Phase 4 begins
└── phase5-engineering-mastery.md      # When Phase 5 begins
```

`overview.md` must contain: feature ID/title, one-paragraph objective and system role, current phase/status, a compact phase status table with **relative links** to existing phase reports, and a short pointer to the next engineer decision or action. It is an **index and handoff**, not a duplicate of detailed phase reports or raw evidence. Phase-specific details, source links, diagrams, decisions, logs and test results belong in that phase's report, with links to evidence when useful. Update the overview's status and links at each phase handoff.

**Markdown linking:** Standard Markdown has links, but no universal file-include directive. Use relative Markdown links (`[Phase 2](phase2-implementation-build.md)`) as the portable default. Only use an include mechanism when the chosen documentation renderer explicitly supports it; never assume ordinary Markdown will inline another file.

**Migration of preexisting combined reports:** For an existing file such as `docs/features/F001-audio-playback.md`, preserve its reviewed content and approved decisions when splitting into the new folder. Create the overview and per-phase reports from existing sections, adjust internal/cross-file links, and retain an informative legacy entry point or redirect-like link until references are updated. Avoid losing history or rewriting verified evidence. Ask before disruptive moves/deletions.

## Progressive disclosure

- **Read the active phase reference:** Phase 1: [references/phase1-requirement-design.md](references/phase1-requirement-design.md). Phase 2: [references/phase2-implementation-build.md](references/phase2-implementation-build.md). Phase 3: [references/phase3-execution-debug.md](references/phase3-execution-debug.md). Phase 4: [references/phase4-verification-review.md](references/phase4-verification-review.md).
- **Phase 5 reference is not yet authored**. Follow the lifecycle and universal rules here until the engineer approves one; do not invent or automatically create a missing reference.
- Read the current feature `overview.md` first. **Phases 1–3** load their active report and only the previous sections needed for decisions. **Phase 4 is an intentional exception:** the independent reviewer must read *all relevant approved Phase 1–3 feature reports*, the implemented design, actual current source and tests, plus bounded related project documents. Reports are inputs to verify, **not assumed proof of code correctness**. Future integration informs boundaries, not scope expansion.
- For stable, repetitive **authorized** operations, prefer existing scripts from `tools/scripts/` to improvised shell commands. Do not require scripts or a custom MCP server to perform a simple task. Specialized skills are optional additions; they cannot supersede engineer approval and safety rules.

## Universal operating rules

1. **Establish scope.** Identify feature ID, active phase, requested deliverable and permitted actions before working. Remain in that feature and phase.
2. **Discover before asking.** Inspect relevant repository material and, when authorized, safe read-only environment information. Ask the engineer only about decisions, access, or genuinely blocking unknowns they must supply.
3. **Distinguish evidence.** Label directly `VERIFIED` facts, `ENGINEER_PROVIDED` information, `INFERRED` conclusions, `UNKNOWN` facts, and `DEFERRED` non-blockers. Never report a test/device check as run unless actually executed.
4. **Control depth by risk and value.** Every investigation must support an approved requirement, a known regression, or a material risk. Prioritize impact and likelihood over finding count, re-use existing tests/evidence, and recommend the smallest verification set sufficient for the feature. Do not escalate minor/theoretical coverage gaps into mandatory tests without a concrete risk rationale. **A test PASS requires an explicit oracle:** the expected behavior, where the expected value came from, the actual result and inspectable evidence; exit code 0 alone never proves audio quality.
5. **Do not leak between phases.** Phase 1 is inspection/design only by default: no product code modifications, build/deploy/run or hardware reconfiguration. Its assigned design report may be written. **Phase 4A is risk-based inspection/review only** (its separate report may be written). **Phase 4B may create small, reusable test scripts/fixtures and execute only Engineer-approved cases**, using Python, C++, shell, existing CTest, or suitable tools. Never modify production code; changes to existing test expectations, existing tests or build configuration require separate approval. Later phases have their own permissions.
6. **Keep debugging human-led and verification independent.** For substantive Phase 3 errors, report symptom and raw evidence first; invite Engineer hypotheses and let them approve experiments; trivial authorized fixes are permitted in Phase 3. **The Phase 4 reviewer never silently fixes production code or self-certifies it**; report defects for Engineer disposition and implementation-owner correction.
7. **Respect target and repo boundaries.** Prior approval is needed for risky changes, destructive commands, device configuration, flashing, reboot, or unauthorized remote operations. Preserve unrelated edits; review diffs, keep commits focused when authorized, and do not push/reset/delete without approval.
8. **Maintain useful docs, not paperwork.** Write each phase report in its own file and keep the feature overview as a compact index. Link to existing evidence and files rather than duplicating large logs. Record design rationale, tests, major debug conclusions and decisions proportionately.
9. **Verify learning with the engineer.** In Phase 5, ask them to explain key code/data flow, design trade-offs and debug approach without simply repeating AI explanations. Identify core knowledge gaps and help practice; noncritical topics may be deferred with reasons, but **critical unverified knowledge blocks mastery sign-off**.
10. **Skill maintenance is separate.** When asked to create or revise skills, modify only the authorized skill files and stop. Never automatically begin a feature phase as a side effect of skill maintenance.

## Handoff and closure

At the end of **each** phase (and at the **Phase 4A sub-gate**), update the separate phase report and `overview.md`, then provide a concise handoff: feature/phase, completed work, observed evidence and relevant paths, unknowns or blockers, specific engineer decisions, and exactly one next suggested action. State one of `READY FOR REVIEW`, `NEEDS ENGINEER DECISION`, or `BLOCKED`; after explicit engineer approval, record `APPROVED/PASS` for that phase. Stop before the next phase unless separately authorized.

Track **two separate final outcomes**:
- **Product verification (Phase 4):** independent review, Engineer-approved verification plan and execution evidence, defects or limitations; **4A requires explicit approval before 4B**.
- **Engineering mastery (Phase 5):** engineer-confirmed understanding of core concepts, code flow and debugging; any approved deferred learning.

Feature is **CLOSED** only after Phase 5 is accepted and product requirements are verified (or deviations explicitly accepted by the engineer). Never self-certify engineer mastery.
