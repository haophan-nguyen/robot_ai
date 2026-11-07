# Phase 3 — Execution & Debug Report

[F001 — Audio Playback overview](overview.md) · [Phase 2 build evidence](phase2-implementation-build.md) · [Phase 1 hardware preflight](phase1-requirement-design.md#hardware-preflight)

## Target and artifact

- **VERIFIED:** Raspberry Pi 3 Model B running Raspbian GNU/Linux 12 (Bookworm), kernel `6.12.87+rpt-rpi-v7`, and `armhf` userspace. The session ran as `mypi`.
- **VERIFIED:** Reused the Phase 2 Release artifact `build-armhf/ai_robot_play_wav`. Local `file` and `readelf` confirmed ARMv7 ELF32, EABI5 hard-float, and interpreter `/lib/ld-linux-armhf.so.3`. Build ID `6a960e3b719f7f3c8c83b3b19059409b5df59188` matches the Phase 2 build.
- **VERIFIED:** `/proc/asound/cards` identified `wm8960soundcard` and `aplay -L` listed `plughw:CARD=wm8960soundcard,DEV=0`. Python's `wave` module read the full sample payload of the installed `/usr/share/sounds/alsa/Front_Center.wav` and confirmed signed PCM16, mono, 48 kHz, 68,545 frames, 1.428 seconds.

## Deployment evidence

```bash
bash tools/scripts/deploy.sh --execute build-armhf/ai_robot_play_wav \
    mypi@10.10.10.2 /home/mypi/ai-robot-f001-phase3.rNCIWFvu/ai_robot_play_wav
```

**PASS.** Transfer completed at 100% with exit code 0. The remote copy was 19,104 bytes, owned by `mypi`, and executable with mode `755`. The local SHA-256 was `829bf3d16ab046bf32c01c5c47625ea2d0e605f44809ec5359104368045d45ae`. Remote `sha256sum -c` reported `OK` against that digest before execution.

## Execution evidence

```bash
/home/mypi/ai-robot-f001-phase3.rNCIWFvu/ai_robot_play_wav \
    --device plughw:CARD=wm8960soundcard,DEV=0 /usr/share/sounds/alsa/Front_Center.wav
```

| Check | Result | Evidence |
| --- | --- | --- |
| Application process | PASS | Exit `0`, stdout empty, stderr empty. The SSH wrapper also exited `0`. |
| WAV and PCM selection | VERIFIED | The application ran with the validated fixture and explicit WM8960 PCM above. The wrapper measured 2,045 ms around the application process. |
| Audible playback quality | FAIL | **Engineer confirmation: YES, with an artifact.** The Engineer heard “Front Center” through WM8960 and later clarified that the final “-ter” in “center” clipped or stuttered. The Engineer reported that this did not occur with the Phase 1 `aplay` baseline. |

## Debug incidents

### L1 — Clipped or stuttered clip ending

**Bug:** On the original player, the last “-ter” in the installed “Front Center” clip was clipped or stuttered through the WM8960. It reproduced with the same WAV and PCM while the process returned exit 0. The Engineer heard a clean ending with `aplay`.

**Fix and status:** F001 now calculates a silent tail from the negotiated ALSA period before draining. The Engineer confirmed a clean ending on the original, unmodified WAV after the rebuild. This mitigates the reported case. The exact cause of the original artifact remains unverified. [Implementation and retest](#approved-application-change-and-original-fixture-retest)

- **ENGINEER_PROVIDED:** The final “-ter” was clipped or stuttered during F001 playback. The Engineer heard a clean ending with [the Phase 1 `aplay` baseline](phase1-requirement-design.md#hardware-preflight), which used the same WAV and PCM.
- **VERIFIED:** F001 exited `0` with empty stdout and stderr. Those process results do not explain the audible artifact.
- **Hypotheses:** F001 may be mishandling the final frames on the target, or its PCM buffer and drain behavior may differ from `aplay` near the end of the stream. The cause is unverified.
- **Engineer decision:** Run the same-file, same-PCM comparison below.

#### Controlled comparison

The Engineer first reported reproducing the clipped ending in repeated F001 runs. The Engineer then authorized a back-to-back comparison. Both commands used the installed 68,545-frame WAV, the same explicit WM8960 PCM, and the existing mixer settings. The deployed F001 binary still matched the Phase 2 SHA-256.

```bash
aplay -v -D plughw:CARD=wm8960soundcard,DEV=0 /usr/share/sounds/alsa/Front_Center.wav
/home/mypi/ai-robot-f001-phase3.rNCIWFvu/ai_robot_play_wav \
    --device plughw:CARD=wm8960soundcard,DEV=0 /usr/share/sounds/alsa/Front_Center.wav
```

The harness ran `aplay` first and F001 after a five-second pause. It captured the processes' output and sampled `/proc/asound/card2/pcm0p/sub0/{hw_params,sw_params,status}` about every 40 ms.

| Evidence | `aplay -v` | F001 |
| --- | --- | --- |
| Exit and logs | Exit `0`. Empty stdout. Stderr reported WAV metadata, mono-to-stereo conversion, and PCM setup. No error diagnostic. | Exit `0`. Empty stdout and stderr. |
| Hardware stream | S16_LE, stereo, 48 kHz, MMAP interleaved | S16_LE, stereo, 48 kHz, MMAP interleaved |
| Buffer / period | 24,000 / 6,000 frames (500 / 125 ms) | 9,600 / 2,400 frames (200 / 50 ms) |
| Last sampled drain status | `appl_ptr=72000`, `hw_ptr=71224`, `delay=776` frames | `appl_ptr=68545`, `hw_ptr=68772`, `delay=0` frames |
| Engineer listening result | Clean final “-ter” | Final “-ter” clipped or stuttered |

Both players reached `RUNNING` and `DRAINING`. The reported pointers are snapshots. F001's application pointer matched the WAV frame count. The `aplay` pointer was 3,455 frames higher, about 72 ms at 48 kHz. The ALSA utils 1.2.8 source shows that `aplay` pads a short final write with silence to its chunk size, consistent with the observed pointer. Both the tail padding and buffer/period differ, so the comparison does not isolate the cause. [ALSA utils 1.2.8 playback source](https://github.com/alsa-project/alsa-utils/blob/v1.2.8/aplay/aplay.c#L2020-L2027)

**Outcome at this step:** The artifact reproduced only with F001. The Engineer approved the trailing-silence experiment below.

#### Trailing-silence experiment

The Engineer authorized comparing the original WAV with a silence-padded copy using the same F001 binary and PCM. The harness preserved all 68,545 original mono PCM16 frames byte for byte, then appended 3,455 zero frames (71.979 ms at 48 kHz). The installed fixture and deployed binary matched their prior SHA-256 values. The harness removed the temporary copy after the two runs.

Both runs selected `plughw:CARD=wm8960soundcard,DEV=0` and negotiated the same hardware stream and thresholds: stereo S16_LE at 48 kHz, 9,600-frame buffer, 2,400-frame period, and 9,600-frame start and stop thresholds. Both reached `RUNNING` and `DRAINING`.

| Evidence | Original WAV | Padded copy |
| --- | --- | --- |
| Source frame count | 68,545 | 72,000 |
| Last sampled drain status | `appl_ptr=68545`, `hw_ptr=69316`, `delay=0` | `appl_ptr=72000`, `hw_ptr=70856`, `delay=1144` |
| Application outcome | Exit `0`, empty stdout and stderr | Exit `0`, empty stdout and stderr |
| Engineer listening result | Final “-ter” clipped | Final “-ter” clean |

**Outcome at this step:** The padded copy sounded clean, and the original still clipped with the pre-fix binary. The padded length also landed on a 2,400-frame period boundary, so this test could not distinguish the effect of trailing silence from the effect of period alignment.

#### Approved application change and original-fixture retest

The Engineer authorized moving the successful fixture padding into F001 and replaying the original WAV. The application queries the negotiated ALSA period with `snd_pcm_get_params`. After submitting every decoded frame, it writes enough silent frames to reach a period boundary with at least one full period of silence, then drains. The same bounded sample buffer and partial-write handling serve both audio and silence. A failed period query or zero period returns a diagnostic. [ALSA PCM API](https://www.alsa-project.org/alsa-doc/alsa-lib/group___p_c_m.html)

The application calculates the silent tail as `period + (period - (wav_frames % period)) % period`, where `period` is the negotiated frame count. For the original fixture, 68,545 WAV frames leave 1,345 frames in the final partial period. The player adds 1,055 frames to reach the boundary and one more silent period of 2,400 frames. It then drains after submitting 72,000 total frames. The calculation uses the PCM settings at runtime. [Playback source](../../../software/src/audio/wav_playback.cpp#L68)

```bash
sysroot="$HOME/.local/share/robot-ai/toolchains/raspbian-bookworm-armhf/sysroot"
bash tools/scripts/build-armhf.sh --sysroot "$sysroot" --build-dir build-armhf-f001-tail \
    --target ai_robot_play_wav -- -DAI_ROBOT_ENABLE_AUDIO_PLAYBACK=ON
bash tools/scripts/deploy.sh --execute build-armhf-f001-tail/ai_robot_play_wav \
    mypi@10.10.10.2 /home/mypi/ai-robot-f001-phase3.rNCIWFvu/ai_robot_play_wav-tail-a45cc32e6e96
# On the Pi:
/home/mypi/ai-robot-f001-phase3.rNCIWFvu/ai_robot_play_wav-tail-a45cc32e6e96 \
    --device plughw:CARD=wm8960soundcard,DEV=0 /usr/share/sounds/alsa/Front_Center.wav
```

| Check | Actual result |
| --- | --- |
| Host CTest | 3/3 PASS, including M0, positive and negative playback cases, and byte-exact stereo samples followed by 2,747 complete silent frames in the file sink. |
| ARM rebuild and transfer | PASS. ELF32 ARMv7, EABI5 hard-float, loader `/lib/ld-linux-armhf.so.3`. Build ID `e298fbe12633ba83ba866f6c75bc1529b033a2c8`. Transfer exited `0`. The deployed SHA-256 matched the new build, `a45cc32e6e9623236130b785f76eeefe8c909b18034ef4f33ae13d6a5481f5f4`. |
| Original fixture | The installed WAV matched its previous SHA-256, `0d61518bcd3f13b0c709a5298e939caf698b80d31d71d50475365ee0e5536cc9`. It still contained 68,545 frames. |
| Original WAV on WM8960 | Exit `0`, empty stdout and stderr. ALSA kept the 9,600-frame buffer and 2,400-frame period. The last drain sample showed `appl_ptr=72000`, `hw_ptr=70620`, `delay=1380`, consistent with 3,455 frames of application-added silence. |
| Engineer listening result | **PASS.** The Engineer confirmed the final “-ter” sounded clean from the original WAV with the rebuilt player. |

**Outcome:** The period-derived padding mitigated the reported clipping on the original fixture. The underlying cause has not been isolated, and this run establishes playback quality for the original 48 kHz mono fixture only.

## Outcome and handoff

**Target smoke PASS with the rebuilt binary · Phase 3 READY FOR REVIEW.** The Engineer confirmed clean playback of the original 48 kHz mono WAV after the approved mitigation. The full target format matrix remains for Phase 4.

**Next action.** Engineer review of the Phase 3 fix and retest before separately authorizing Phase 4.
