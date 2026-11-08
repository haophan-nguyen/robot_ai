# Phase 3 — Execution & Debug Report

**F002 — Audio Capture · APPROVED / PASS · 2026-11-08**

[Feature overview](overview.md) · [Approved Phase 2 build](phase2-implementation-build.md) · [Phase 1 preflight](phase1-requirement-design.md#observed-preflight-evidence)

## 1. Target and approved artifacts

The approved Phase 2 baseline is Git commit `c0bd8fcf85abb85eb709aa4d504ecf2e563694f5`. Phase 3 reused the ARM Release binary from that baseline. The Pi reported Raspbian GNU/Linux 12 (Bookworm), kernel `6.12.87+rpt-rpi-v7`, `armv7l`, and `armhf` userspace. `/proc/asound/pcm` and `arecord -l` identified WM8960 card 2, device 0, with one capture and one playback subdevice. Read-only mixer inspection showed both Capture channels on at +12 dB and both ADC PCM channels at 0 dB. Both applications used `plughw:CARD=wm8960soundcard,DEV=0`. [Target transcript](../../../build-f002-phase3/target-evidence.log)

| Artifact | Identity |
| --- | --- |
| F002 recorder | Local `build-f002-phase2/armhf/ai_robot_record_wav`. Local and deployed SHA-256 both `163efb05f06240c411fea50f6e515b6475b994e371cdee36fc4c7859a2726313`. |
| F001 player | `/home/mypi/ai-robot-f001-phase3.rNCIWFvu/ai_robot_play_wav-chunk-e60ebc2c06b0`. The existing Pi copy matched the F001 verification SHA-256 `e60ebc2c06b0f52616ebb0405ced47e9467c1e050ab52ea7d2a37218c332dbc4`. [F001 target verification](../F001-audio-playback/phase4-verification-report.md#mvp-02--raspberry-pi--wm8960-listening--pass) |
| Start cue | Installed `/usr/share/sounds/alsa/Front_Center.wav`, SHA-256 `0d61518bcd3f13b0c709a5298e939caf698b80d31d71d50475365ee0e5536cc9`, matching the previously verified F001 fixture. |

## 2. Deployment

**PASS.** The existing deploy script previewed the destination, then transferred the recorder using SCP with host-key checking. Transfer returned 0. The Pi copy was 23,980 bytes, mode `755`, owned by `mypi`, and executable. Its digest matched the approved local artifact. The recording path was absent before the run. [Deploy script](../../../tools/scripts/deploy.sh) · [Transfer and hash evidence](../../../build-f002-phase3/target-evidence.log)

```bash
artifact=build-f002-phase2/armhf/ai_robot_record_wav
destination=/home/mypi/ai-robot-f002-phase3.RwiCmF0S/ai_robot_record_wav
bash tools/scripts/deploy.sh --dry-run "$artifact" mypi@10.10.10.2 "$destination"
bash tools/scripts/deploy.sh --execute "$artifact" mypi@10.10.10.2 "$destination"
```

## 3. Target execution

The Engineer submitted **Start** from the microphone and speaker. F001 played “Front Center” as the cue. F002 then recorded one take. Python's `wave` reader checked format, frame count, and the complete payload before F001 replayed the recording.

```bash
pcm='plughw:CARD=wm8960soundcard,DEV=0'
binary=/home/mypi/ai-robot-f002-phase3.RwiCmF0S/ai_robot_record_wav
player=/home/mypi/ai-robot-f001-phase3.rNCIWFvu/ai_robot_play_wav-chunk-e60ebc2c06b0
wav=/home/mypi/ai-robot-f002-phase3.RwiCmF0S/f002-capture.wav

timeout -k 2s 20s "$player" --device "$pcm" /usr/share/sounds/alsa/Front_Center.wav
timeout -k 2s 20s "$binary" --device "$pcm" "$wav"
# The WAV check completed before replay.
timeout -k 2s 20s "$player" --device "$pcm" "$wav"
```

| Check | Expected | Actual |
| --- | --- | --- |
| Start cue | F001 exit 0 before capture. | Exit 0, empty stdout and stderr. |
| F002 recording · R1–R3 | Exit 0 through the explicit WM8960 PCM and publish one complete WAV at the fresh path. | Exit 0 in 5,459 ms wall time. Stdout and stderr were empty. The output directory contained one WAV and no temporary capture sibling. |
| WAV metadata and payload · R2–R3 | RIFF/WAVE PCM16, mono, 48,000 Hz, 240,000 frames and 480,000 readable sample bytes. | Python `wave` exited 0 with `('NONE', 1, 2, 48000, 240000, 480000)` for encoding, channels, bytes per sample, rate, frames, and payload bytes. File size was 480,044 bytes and mode was `600`. Sample range was −32,758 to 19,268, with zero full-scale samples. |
| F001 replay · R4 | Replay the captured WAV through WM8960 with exit 0. | Exit 0 in 5,564 ms wall time. Stdout and stderr were empty. |
| Engineer listening · R4 | Full phrase at normal pace with the final word intact and no obvious clipping or dropouts. | **YES · ENGINEER_PROVIDED.** The Engineer selected “Yes, the full phrase sounded clear” in response to that check. |

The Pi retained the WAV and per-stage `.stdout` and `.stderr` files in `/home/mypi/ai-robot-f002-phase3.RwiCmF0S`. The WAV SHA-256 was `843ea8f45712645ebbea2641ec33069af032c285d8ff06ef423a956e58eefded`. [Stage statuses and output](../../../build-f002-phase3/target-evidence.log)

## 4. Outcome and handoff

**APPROVED / PASS.** The Engineer approved Phase 3 on 2026-11-08 after deployment, recording, WAV checks, F001 replay, and listening met the target smoke criteria. The run exercised one capture and replay on the approved route. Independent verification belongs to Phase 4.

Next Engineer action. Authorize Phase 4A when ready.
