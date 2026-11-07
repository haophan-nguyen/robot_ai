# Phase 1 — Requirement & Design (V2.1)

## Purpose and boundary

Produce an engineer-reviewable design for the **current feature**. This is **inspection and design**, not implementation. Source code, build configuration, deployments, hardware state, and target execution remain unchanged by default. Updating the assigned Phase 1 design report is permitted. Inspection of a remote device must be read-only and within granted access; otherwise record the unverified condition.

## Input: lightweight Engineer Brief

Accept a short statement of:
- Feature ID/name and expected behavior.
- Known target/hardware constraints and any non-negotiable requirements.
- Existing design links or prior decisions, if available.

Do **not** require the engineer to supply detailed APIs, scripts, implementation choices, or a complete design first. Discover project background from `docs/project/vision.md`, `docs/project/roadmap.md`, `docs/project/backlog.md`, relevant `docs/architecture/`, `docs/decisions/`, source, and existing feature documents **selectively**.

## Processing workflow

1. **Project & feature context:** Identify long-term vision, immediate MVP, owner subsystem, interfaces/dependencies, and likely future integration. Distinguish current scope from future possibilities.
2. **Targeted discovery:** Read relevant code/config/docs. Verify tool/device facts via non-invasive, authorized checks when useful. Cite paths, commands, and observed outputs in the report. Don't assume that past success proves current target readiness.
3. **Information ledger:** Label each relevant assertion as:
   - `VERIFIED` — directly observed with evidence/path;
   - `ENGINEER_PROVIDED` — supplied by engineer, not yet rechecked;
   - `INFERRED` — reasoned but unverified, with rationale;
   - `UNKNOWN` — genuinely missing;
   - `DEFERRED` — outside current blocking scope, with reason.
4. **Gap resolution:** Self-check what is safe and knowable. For a requirement/design decision, suggest a sensible default, trade-off, and its effect, then ask the engineer. Never convert guesses into decisions. Separate blocking gaps from nonblocking future questions.
5. **Design proposal:** Define behavior, main module boundary, data flow, minimal interfaces, error paths, dependencies, implementation sequence, and testable acceptance criteria. Do not overengineer or build an entire future framework.
6. **Stop condition:** Once the report is sufficient to make the next design decision, stop further investigation. Every additional investigation must serve a stated requirement, decision, or risk. Do not ask repetitive/self-evident questions.

## Output: seven-section Phase 1 report

Create/update **`docs/features/<feature-id>-<short-slug>/phase1-requirement-design.md`** as the *Phase 1-only* report. Create/update **`docs/features/<feature-id>-<short-slug>/overview.md`** as the short feature index, status tracker, and entry point linking the Phase 1 report. If writing is not permitted, present the proposed report in-chat and mark it not saved. Keep all seven Phase 1 sections, but adjust depth to complexity; short tables/links are fine.

Never append subsequent phase reports to `phase1-requirement-design.md`. If an older combined feature report already exists, preserve evidence and approved decisions when migrating; keep a discoverable link from its old path until repository references have been updated.

| # | Section | Minimum required content |
|---|---|---|
| 0 | **Project Context & Vision** | Purpose of project/MVP, owning subsystem, feature's role, future connections, what is not being built now |
| 1 | **Feature Understanding** | Goal, user-visible behavior, explicit in-scope/out-of-scope |
| 2 | **Context & Information** | Evidence-labeled known facts, missing info, blocking vs deferred gaps; relevant source links |
| 3 | **Technical Design** | Proposed module/interface/data flow, important options and trade-offs, error behavior |
| 4 | **Execution Plan** | Bounded implementation steps for Phase 2 and dependencies, without executing them |
| 5 | **Verification & Risks** | Measurable acceptance criteria, positive/negative tests, relevant risks and mitigations |
| 6 | **Decision & Readiness** | Minimal engineer decisions with recommendations; status and exact next action |

Avoid repeating the same project background across sections. Distinguish a proposal from an approved design. Explain major decisions concisely rather than offering a long list of speculative alternatives.

## Decision gate

Choose exactly one current status:
- **READY FOR REVIEW:** Sufficient proposed design, test criteria, and no unresolved critical investigation blockers; engineer approval still required.
- **NEEDS ENGINEER DECISION:** A bounded, material requirement/design choice must be made; give options and a recommended default.
- **BLOCKED:** A critical prerequisite cannot be established safely with available access; state the concrete blocker and resolution.

**Phase 1 PASS / APPROVED** requires explicit engineer acceptance of the final report/design. Update the overview status and link at handoff. A `READY FOR REVIEW` label is never permission to start Phase 2. Stop after reporting and asking only the essential decisions. If the engineer revises the design, incorporate that feedback before requesting approval again.

## F001 grounding example (not a mandatory design)

For Raspberry Pi + WM8960 Audio Playback, treat the engineer's statement that the sound card previously played audio successfully as `ENGINEER_PROVIDED` historical context, not `VERIFIED` current device state. The agent should inspect project build structure and related existing documentation; propose a minimal WAV/PCM playback scope and validation plan. The exact ALSA device name, supported PCM formats, and current hardware readiness stay `UNKNOWN` until observed in an authorized read-only check. Do not implement playback or create an AudioManager framework in Phase 1.
