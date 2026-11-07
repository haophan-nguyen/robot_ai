# Phase 3 — Target Execution & Engineer-Led Debug (V2.2)

## Purpose and preconditions

Execute the **Phase 2-built application on the approved Raspberry Pi target**, establish actual behavior, isolate failures, and return useful evidence to the engineer. This is a **target bring-up and diagnosis phase**, not final requirement-coverage verification (Phase 4) or engineering mastery sign-off (Phase 5).

**Start only after the engineer explicitly approves Phase 2 and authorizes Phase 3 target execution.** Read the feature `overview.md`, approved Phase 1 design, Phase 2 implementation/build report, and relevant verified hardware preflight results. Follow the approved feature scope; do not start Phase 4 automatically.

For F001, initial target smoke is one known-good WAV through the verified WM8960 ALSA PCM. The user must confirm audible output. The four WAV format combinations and comprehensive negative-case coverage are tracked for Phase 4 unless separately authorized in Phase 3.

## Operational boundaries

- **Required deployment entry point:** `tools/scripts/deploy.sh` (repository-relative; its source is currently under `/workspace/robot_ai/tools/scripts/deploy.sh`). **Inspect the script and its documented arguments before invoking it.** Do not assume that its positional arguments, destination, or behavior are known; do not create a competing deploy script. If the script would perform unapproved actions, stop and ask.
- Reuse the existing Phase 2 ARM artifact produced through `tools/scripts/build-armhf.sh`. Do not rebuild automatically unless a verified stale/missing artifact requires a permitted rebuild. Check artifact identity and target ARM ELF/ABI; record the executable path and, where feasible, local/remote SHA-256, executable bit, and target destination under the Pi user's home directory.
- Authorized: read-only target inspection, deployment into a dedicated non-system directory, running the F001 executable as the ordinary Pi user, collecting logs/status, and **narrow L0 fixes** defined below.
- Not authorized by default: `sudo`/package installation, changing mixer/device tree/driver/kernel, reboot, system configuration changes, deleting unrelated target files, changing approved requirements, wide source refactors, or executing unrelated features. Never store credentials in scripts or reports.
- If the deployed script also **runs** the application, identify that behavior in advance and execute it only under the Phase 3 permission. Use approved test media and conservative audio output; do not change mixer volume to make a test pass.

## Step-by-step execution

1. **Preflight:** Check approved target identity/access, Phase 2 artifact, deploy script behavior and required parameters, existing tested ALSA PCM, WAV metadata, and expected log/exit status. Prefer the Phase 1 known-good hardware fixture; verify current device state before assuming past settings still apply.
2. **Deploy:** Call the existing `tools/scripts/deploy.sh` with verified arguments. Record command (redact secrets), destination, transfer status, and artifact/hash evidence. Do not silently overwrite unrelated deployments.
3. **Execute:** Run the deployed binary via SSH as the Pi user with the selected `--device` and known-good WAV. Capture stdout, stderr, exit code, exact invocation, source fixture, and relevant target diagnostics. Distinguish application output from SSH wrapper or `aplay` results.
4. **Engineer hearing confirmation:** After playback, explicitly ask **“Did you hear the expected audio through the WM8960 speaker, at normal speed and without obvious interruption?”** Accept yes/no and observations. Do not infer audible success from exit code, ALSA drain, or a past `aplay` PASS.
5. **If failure:** Preserve evidence; classify L0/L1/L2 using the matrix below. Follow the corresponding authority and retry rules. Prefer discriminating tests over speculative modifications.
6. **Document and stop:** Write the separate `phase3-execution-debug.md` report and update `overview.md`. State `READY FOR REVIEW`, `NEEDS ENGINEER DECISION`, or `BLOCKED`; request engineer review. No Phase 4 without distinct approval.

## Failure classification and permissions

**Classify by actual evidence and impact, not merely by the error message. If unsure whether a failure is L0, treat it as L1.**

| Level | What it means and examples | Agent authority |
|---|---|---|
| **L0 — Routine / mechanical** | Clear, localized operational mistake with a verified remedy: typo in already-known WAV or PCM argument; wrong deployment path; missing executable bit in dedicated deploy directory; incorrect script parameter; recoverable SSH/rsync invocation issue. | May correct **only the specific script/argument/deployed user-owned file** after collecting evidence. Explain the fix, rerun bounded checks, and report changes. Do not silently modify application logic. |
| **L1 — Technical / uncertain cause** | ALSA `snd_pcm_open`/configuration failures with uncertain cause; `snd_pcm_writei` errors, underrun, partial write mishandling; missing dynamic library/ABI or runtime mismatch not trivially proven; silence despite exit 0; wrong speed, clipping, truncation, unexpected crash, invalid data flow. | **Evidence collection and read-only diagnosis are allowed.** Report symptoms, exact logs, 1–3 plausible hypotheses, and the smallest discriminating experiment. **Engineer selects/approves experiments and any code or build-system fix.** Agent must not autonomously modify source or target configuration. |
| **L2 — System / high impact** | Fix would require package installation on Pi, privileged changes, mixer or driver/device-tree/kernel modification, reboot, overwriting system paths, broad/destructive operations, or could adversely affect hardware. | **Stop immediately.** Explain impact, capture safe evidence, request explicit separate approval. Never escalate a L1 into L2 activity on your own. |

**Important distinctions:** A verified typo in the PCM string may be L0; a correctly named PCM that cannot open is L1. A missing `.so` on Pi is at least L1 until compatibility is understood; installing it is L2. `aplay` working does not prove F001 is correct; F001 silent with successful exit remains L1 until understood.

### Debug loop and stop rules

- For each issue record: **symptom → observed evidence → classification → hypothesis → single proposed experiment/fix → outcome**. Distinguish observations from assumptions. Save raw output or a bounded excerpt with command and path; avoid giant logs in the report.
- **L0 autonomy:** maximum **3 corrective retry attempts per specific issue**, each grounded in a different observation or meaningful fix; do not repeat identical failed commands without a reason. This retry limit does **not** restrict planned verification cases. If uncorrected, ambiguous, or evolving into a technical issue, stop and return to engineer rather than exhausting attempts.
- **L1 technical debugging:** Agent should first collect non-mutating evidence (e.g., PCM inventory, exit status, `file`/`readelf` dependency clues, code path, existing baseline comparison) and propose focused hypotheses. **Wait for engineer instruction before running a new diagnostic experiment that changes program behavior, modifying implementation, or changing the target.** Simple authorized read-only inspection does not require a new approval every command.
- **L2:** no automated fixes, retries or modifications. Ask for permission with risk and rollback considerations.
- After any approved fix, reproduce the original failure, rerun the relevant smoke test, document changed files and observed effect. Do not declare a root cause when only a workaround or correlation is known.

## F001 initial success criteria

| Check | Evidence required |
|---|---|
| Deployment | Existing script completes; target path and artifact identity recorded (hash when feasible) |
| Target execution | F001 executable runs on Raspberry Pi as ARM armhf; actual exit status and stdout/stderr captured |
| Playback route | Executable explicitly selects verified WM8960 PCM and a valid approved test WAV |
| Audible playback | Engineer confirms expected audible output; log exit code alone is insufficient |
| Failure handling | Any problem is classified and handled within L0/L1/L2 authorization |

**A Phase 3 smoke PASS is not Phase 4 product-verification PASS.** Do not claim all WAV combinations, negative cases, reliability, or long-term streaming work on the Pi unless those checks were actually run.

## Required output — separate Phase 3 report

Create `docs/features/<feature-id>-<short-slug>/phase3-execution-debug.md` and link it in `overview.md`. Keep the report short for an uneventful PASS and expand only for meaningful failures. Include:

1. **Target and artifact:** OS/architecture when relevant, binary path, build/version/commit identity, ALSA PCM, WAV fixture and parameters.
2. **Deployment evidence:** exact sanitized deploy invocation, destination, transfer result/hash or why unavailable.
3. **Execution evidence:** exact command, actual stdout/stderr excerpts, exit code, engineer hearing confirmation (`YES` / `NO` / `PENDING`).
4. **Debug incident log (if applicable):** L0/L1/L2 level, symptoms, hypothesis, experiment/changes, retry count, result, engineer decisions. Link to detailed logs rather than pasting them wholesale.
5. **Outcome and handoff:** PASS/FAIL/BLOCKED per check, known limitations, unresolved issues, `READY FOR REVIEW` / `NEEDS ENGINEER DECISION` / `BLOCKED` and one proposed next action.

**Never** fabricate commands, results, user hearing confirmation, root cause, successful audio, or Phase 4 verification. The engineer alone approves Phase 3 completion and separately authorizes Phase 4.
