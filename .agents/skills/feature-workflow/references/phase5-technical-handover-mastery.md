# Phase 5 — Technical Handover & Engineering Mastery (V2.6)

## Purpose and prerequisites

Phase 5 completes one approved feature with **(A) one reliable, standalone technical handover another engineer can follow** and **(B) exactly five core questions that reveal whether the Engineer understands the feature**. This phase is a **knowledge-consolidation and handoff task**, not an invitation to implement more features, expand tests, repeat a full code audit, or add paperwork.

Start only after the Engineer has explicitly accepted **Phase 4 product verification** and authorized Phase 5. The Phase 4 PASS and any accepted residual risks are the verification baseline, not permission to invent additional PASS results. If the selected feature/revision is ambiguous, state it and use the approved baseline.

**Allowed changes:** Create/update `docs/features/<feature-id>-<slug>/technical-handover.md`, `phase5-engineering-mastery.md`, and the **short index/link/status** in `overview.md`. Make only minimal corrections to **other documentation** if explicitly approved; otherwise record discrepancies with references and ask. Do **not** edit production code, tests, build/deploy scripts, Phase 1–4 reports, or approved diagrams; do not install packages, build, test, deploy, or change Pi configuration unless separately authorized. **When asked only to update this Skill, edit Skill files and stop.**

## Step 1 — Read and reconcile the complete feature context

This is a *cross-phase* task; do not summarize from `overview.md` or agent conclusions alone. Read the following before drafting:

1. Feature `overview.md` and **all approved Phase 1, Phase 2, Phase 3, Phase 4 reports**, including approved requirements, decisions, debug incident history, product-verification evidence, deferrals and limitations.
2. Current implementation **in full for this feature**, including public interface, entry point, relevant source/header files, CMake/build options, relevant `tools/scripts` and tests/fixtures. Inspect the code path, function contracts and relevant dependencies; stay inside feature-relevant scope, not the entire unrelated robot repository.
3. Relevant design/sequence/PlantUML, feature requirements and project architecture notes. Use the *actual approved final source*, not the older Phase 2 snapshot. Identify revision when available.
4. Where a statement relies on an execution log or test result, follow the Phase report's evidence reference if accessible. Prior reports may be historical; do not pretend to rerun anything in Phase 5.

Resolve **minor wording inconsistencies in the new handover** by anchoring to approved code/evidence. For material contradictions (requirements vs code, approval status, test failures) **do not silently reconcile or falsify history**; list precise source references and request Engineer resolution. Label evidence `VERIFIED` (inspected source/current file), `HISTORICAL` (previous execution record), `ENGINEER_PROVIDED`, `INFERRED`, `UNKNOWN` or `DEFERRED` where clarity matters. Link to relevant source and prior reports with valid relative paths; no invented lines or commands.

## Step 2 — Write ONE complete technical handover document

**Deliverable:** `docs/features/<feature-id>-<slug>/technical-handover.md`. The target reader is an engineer who **did not participate in development** but needs to understand, run, maintain and extend this feature. The handover should be readable independently, with links to detailed evidence rather than copying every phase report or log. Prefer diagrams already approved; add a small text flow when it helps. **Don't force arbitrary length; clarity and reproducibility matter more than size.**

Include the following, adapting the order to the actual feature:

| Section | Content expected from inspected sources |
|---|---|
| **Overview & scope** | Why the feature exists, its system role, implemented behavior, supported/unsupported input/operation, constraints and known non-goals |
| **Architecture & code map** | Actual entry points, key modules/files and responsibilities; public API and useful function inventory (real signatures/locations, parameters, returns/error semantics); relative links to code and diagrams |
| **End-to-end code/data flow** | Trace an ordinary valid request from input to output and cleanup, including important decisions, data ownership, buffer/format conversions and error paths. Explain *why* relevant design choices were made, not just list API calls |
| **Build, run and reproduce** | Verified prerequisites, relevant host/target assumptions, existing scripts and commands with configurable paths/arguments; how to execute a minimal acceptance check. Distinguish historical working steps from unverified current commands; never disclose private credentials or assume machine-specific paths are universal |
| **Verification and evidence** | Phase 4 accepted test groups and actual outcomes, how PASS was established (oracle vs actual vs Engineer hardware confirmation), known limitations, deferred coverage; link to full Phase 4 report, don't paste logs |
| **Debug lessons and tradeoffs** | Relevant actual incident(s), hypotheses/experiments, chosen solution, why alternatives were rejected/deferred, and what root causes remain unconfirmed. Do not turn a successful mitigation into a proven root cause |
| **Maintenance & extension guide** | Where and how another engineer would make a bounded change, relevant assumptions to preserve, sensible regression checks and future extension points without implementing future work |
| **References** | Relative links to overview, Phase 1–4 reports, diagrams, source, tests, tools and evidence |

For F001-like audio features, high-value details can include the actual PCM sample/frame/channel representation, library/ALSA lifecycle, negotiated period vs application chunk, final-chunk silence policy, `snd_pcm_drain`, partial-write handling, format validation, and the distinction between CLI exit success and audible playback. Include such details **only when confirmed in the sources**. Do not invent experimental results, undocumented capabilities or exact code signatures.

**Handover usability check:** Could a different Engineer locate the relevant code, explain the data flow, build/run it in the described environment, reproduce the supported case, understand the known risks, and know where to make the next change? If not, fix the handover itself before claiming readiness. No new test framework or unapproved experimentation is necessary.

## Step 3 — Create SEPARATE Phase 5 report and exactly five core questions

**Deliverable:** `docs/features/<feature-id>-<slug>/phase5-engineering-mastery.md`. This is a Phase report **distinct from the handover**. It must not be appended to earlier reports, must not reproduce the full handover, and must be linked from `overview.md`.

The Phase 5 report has these concise sections:

1. **Subject and source inventory:** Feature ID, approved revision if known, which Phase 1–4 documents and feature source/tests were actually read; list any unavailable material.
2. **Documentation synthesis:** Link to `technical-handover.md`, brief coverage/usability check, essential discrepancies or remaining unknowns only.
3. **Five-question Engineer knowledge check:** **Exactly five**, clear, open-ended, feature-specific questions grounded in the actual source and decisions. Prefer these five capabilities, not a large memorization exam:
   - **Q1 — Purpose and architecture:** What the feature does, main responsibilities and boundaries.
   - **Q2 — Code/data flow:** Walk a normal execution through key functions and buffers from input to output.
   - **Q3 — Critical technical concept:** Explain one core mechanism or API relevant to this feature (e.g., formats, frames/chunks, lifecycle) with correct reasoning.
   - **Q4 — Debug and tradeoff:** Explain a real defect/experiment/solution or a consequential design decision; distinguish evidence from hypothesis.
   - **Q5 — Verification and extension:** Explain how to demonstrate correct behavior and how to approach one realistic, bounded extension without breaking the current feature.
4. **Engineer answer & feedback log:** Initially record all five as **PENDING ENGINEER RESPONSE**. Never fabricate answers, assign a PASS, or pre-fill model answers before the Engineer responds. On a later, explicitly requested response-review turn, assess each response **UNDERSTOOD / PARTIAL / GAP** with short evidence-grounded feedback and at most one focused follow-up for genuine gaps. Do not require knowledge of every minor API or repeat a five-question cycle automatically.
5. **Readiness, deferred learning, approval:** Handover readiness, Engineer learning gaps and decisions; status `READY FOR ENGINEER REVIEW`, `NEEDS ENGINEER DECISION` or `BLOCKED` until Engineer accepts. Note exactly what still requires a decision before closure.

Ask the Engineer to answer the five questions **in their own words**; present them in one compact set (or one at a time if requested), without supplying the answers alongside the questions. The Engineer judges their learning and may accept noncritical deferred topics. A misunderstanding of core code flow or the central feature mechanism requires a focused correction before mastery sign-off; do not substitute an AI-generated explanation for Engineer understanding.

## Step 4 — Close only after Engineer approval

At first Phase 5 execution, write the complete handover, the separate Phase 5 report with **five unanswered questions**, and link both in the short `overview.md`; report `READY FOR ENGINEER REVIEW` and **STOP**. Do not auto-answer the quiz or mark mastery `PASS`.

After the Engineer supplies answers, evaluate them against inspected code and approved documentation, record their responses and concise feedback, then ask only about a material gap. **Feature closure occurs only after explicit Engineer confirmation** that the handover and mastery are accepted and Phase 4 product verification remains approved. Update `overview.md` status to `CLOSED` only then; preserve references to the Phase 4 residual risks and any accepted learning deferrals. Do not automatically begin the next feature.

## Exit checklist

- [ ] Read actual complete feature-relevant source/tests plus **all relevant Phase 1–4 docs** and approved design; note source revision and missing references.
- [ ] One standalone `technical-handover.md` accurately covers scope, code/data flow, build/run, verification evidence, debug decisions, limitations and maintenance handoff; links work.
- [ ] One **separate** `phase5-engineering-mastery.md` records the documentation check, **exactly five** grounded questions and pending (or later real) Engineer responses.
- [ ] `overview.md` remains a compact index with relative links, not an oversized report.
- [ ] No production/test changes, invented test results or premature mastery/feature closure.
- [ ] Stop for Engineer review; no new phase or feature without authorization.
