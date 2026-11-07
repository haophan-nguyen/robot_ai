"""Exercise the F001 CLI with generated WAVs and software ALSA sinks."""

import os
from pathlib import Path
import struct
import subprocess
import sys
from tempfile import TemporaryDirectory
import wave


MISSING_PCM = "__ai_robot_missing_pcm__"
FRAME_COUNT = 2053


def make_wav(path, channels=1, rate=48000, width=2, frames=FRAME_COUNT):
    # Distinct frames and channels make omissions and interleaving errors visible.
    if width == 2:
        samples = (index % 32767 - 16383 for index in range(frames * channels))
        data = struct.pack(f"<{frames * channels}h", *samples)
    else:
        data = b"\0" * (frames * channels * width)
    with wave.open(str(path), "wb") as wav:
        wav.setparams((channels, width, rate, 0, "NONE", "not compressed"))
        wav.writeframes(data)
    return data


def expect(executable, args, exit_code, *diagnostics):
    completed = subprocess.run(
        [executable, *map(str, args)], capture_output=True, text=True, timeout=5
    )
    valid_stderr = all(part in completed.stderr for part in diagnostics) if diagnostics else not completed.stderr
    if completed.returncode != exit_code or not valid_stderr or completed.stdout:
        raise RuntimeError(
            f"{args}: expected exit {exit_code} and stderr {diagnostics!r}, got "
            f"exit {completed.returncode}, stdout {completed.stdout!r}, stderr {completed.stderr!r}"
        )
    print(f"PASS {args}: exit {exit_code}")


def verify_valid(executable, directory):
    for rate in (44100, 48000):
        for channels in (1, 2):
            path = directory / f"pcm16-{rate}-{channels}.wav"
            make_wav(path, channels=channels, rate=rate)
            expect(executable, ["--device", "null", path], 0)

    with wave.open(str(directory / "pcm16-48000-2.wav"), "rb") as stereo:
        data = stereo.readframes(stereo.getnframes())

    # RIFF/WAVE can describe PCM16 via the WAVE_FORMAT_EXTENSIBLE header.
    pcm_guid = bytes.fromhex("0100000000001000800000aa00389b71")
    fmt = struct.pack("<HHIIHHHHI", 0xFFFE, 2, 48000, 192000, 4, 16, 22, 16, 3)
    fmt += pcm_guid
    chunks = b"WAVEfmt " + struct.pack("<I", len(fmt)) + fmt
    chunks += b"data" + struct.pack("<I", len(data)) + data
    extensible = directory / "stereo.data"
    extensible.write_bytes(b"RIFF" + struct.pack("<I", len(chunks)) + chunks)

    # ALSA's file PCM forwards to null and records the data submitted by the CLI.
    recorded = directory / "stereo.raw"
    expect(executable, ["--device", f"file:{recorded}", extensible], 0)
    expected = data
    if sys.byteorder == "big":
        expected = struct.pack(f">{len(data) // 2}h", *struct.unpack(f"<{len(data) // 2}h", data))
    captured = recorded.read_bytes()
    if not captured.startswith(expected):
        raise RuntimeError("ALSA file sink did not receive all stereo samples in order")
    tail = captured[len(expected):]
    bytes_per_frame = struct.calcsize("hh")
    if not tail or len(tail) % bytes_per_frame or any(tail):
        raise RuntimeError("ALSA file sink did not receive a tail of complete silent frames")
    print(f"PASS byte-exact stereo audio followed by {len(tail) // bytes_per_frame} silent frames")


def verify_invalid(executable, directory):
    good = directory / "good.wav"
    make_wav(good)
    expect(executable, [], 2, "Usage:")
    expect(executable, ["--device", "null"], 2, "Usage:")
    expect(executable, ["--device", "", good], 2, "Usage:")
    expect(executable, ["--device", MISSING_PCM, good], 1, "open PCM")

    expect(executable, ["--device", MISSING_PCM, directory / "missing.wav"], 1, "open WAV")
    expect(executable, ["--device", MISSING_PCM, directory], 1, "open WAV")
    unreadable = directory / "unreadable.wav"
    make_wav(unreadable)
    unreadable.chmod(0)
    if not os.access(unreadable, os.R_OK):
        expect(executable, ["--device", MISSING_PCM, unreadable], 1, "open WAV")
    else:
        print("SKIP unreadable file case: this user can read mode-000 files")
    malformed = directory / "malformed.wav"
    malformed.write_bytes(b"not a WAV file")
    expect(executable, ["--device", MISSING_PCM, malformed], 1, "open WAV")

    for name, options, diagnostic in (
        ("empty", {"frames": 0}, "no audio frames"),
        ("24-bit", {"width": 3}, "signed 16-bit PCM"),
        ("three-channels", {"channels": 3}, "unsupported channel count"),
        ("32-khz", {"rate": 32000}, "unsupported sample rate"),
    ):
        path = directory / f"{name}.wav"
        make_wav(path, **options)
        # Invalid file diagnostics must win over failure to open the PCM.
        expect(executable, ["--device", MISSING_PCM, path], 1, "validate WAV", str(path), diagnostic)


if __name__ == "__main__":
    mode, executable = sys.argv[1:]
    with TemporaryDirectory(prefix="ai-robot-f001-") as temporary:
        checks = {"valid": verify_valid, "invalid": verify_invalid}
        checks[mode](executable, Path(temporary))
