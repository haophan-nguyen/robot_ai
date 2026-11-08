"""Check F002 against a deterministic ALSA capture source without a sound card."""

import json
import os
from pathlib import Path
import resource
import signal
import struct
import subprocess
import sys
from tempfile import TemporaryDirectory
import wave


FRAME_COUNT = 240_000


def file_pcm(name, source, sink="/dev/null"):
    return f"""
pcm.{name} {{
    type file
    slave.pcm "test_null"
    file "{sink}"
    infile {json.dumps(str(source), ensure_ascii=False)}
    format "raw"
}}
"""


def limit_output_size():
    # The WAV header and first blocks fit before the kernel reports a write failure.
    limit = 32 * 1024
    resource.setrlimit(resource.RLIMIT_FSIZE, (limit, limit))
    signal.signal(signal.SIGXFSZ, signal.SIG_IGN)


def expect(executable, environment, args, exit_code, diagnostic=None, *, preexec_fn=None):
    completed = subprocess.run(
        [executable, *map(str, args)],
        env=environment,
        capture_output=True,
        text=True,
        timeout=10,
        preexec_fn=preexec_fn,
    )
    stderr_matches = diagnostic in completed.stderr if diagnostic else not completed.stderr
    if completed.returncode != exit_code or completed.stdout or not stderr_matches:
        raise RuntimeError(
            f"{args}: expected exit {exit_code} and diagnostic {diagnostic!r}, got "
            f"exit {completed.returncode}, stdout {completed.stdout!r}, stderr {completed.stderr!r}"
        )


def verify_capture(executable, directory):
    samples = [(index * 257) % 65536 - 32768 for index in range(FRAME_COUNT - 3)]
    samples.extend((12345, -23456, 32767))
    source = directory / "source.raw"
    source.write_bytes(struct.pack(f"={FRAME_COUNT}h", *samples))

    config = directory / "alsa.conf"
    config.write_text(
        "pcm.test_null { type null }\n"
        + file_pcm("test_capture", source)
        + file_pcm("test_capture_read_error", source, "/dev/full"),
        encoding="utf-8",
    )
    environment = dict(os.environ, ALSA_CONFIG_PATH=str(config))
    outputs = directory / "outputs"
    outputs.mkdir()
    recorded = outputs / "recorded.wav"
    expect(executable, environment, ["--device", "test_capture", recorded], 0)
    with wave.open(str(recorded), "rb") as wav:
        actual_format = (wav.getnchannels(), wav.getsampwidth(), wav.getframerate(),
                         wav.getnframes(), wav.getcomptype())
        expected_format = (1, 2, 48_000, FRAME_COUNT, "NONE")
        if actual_format != expected_format:
            raise RuntimeError(f"expected WAV format {expected_format}, got {actual_format}")
        payload = wav.readframes(FRAME_COUNT + 1)
        if payload != struct.pack(f"<{FRAME_COUNT}h", *samples):
            raise RuntimeError("recorded payload differs from the generated PCM16 input")
    print("PASS 240,000 mono PCM16 frames at 48 kHz and byte-exact 480,000-byte payload")

    absent = outputs / "absent.wav"
    expect(executable, environment, [], 2, "Usage:")
    expect(executable, environment, ["--device", "missing_pcm", absent], 1, "open PCM")
    # The ALSA file plugin reports a capture read error when its sink fails.
    expect(executable, environment, ["--device", "test_capture_read_error", absent], 1, "read PCM")
    expect(executable, environment, ["--device", "test_capture", absent], 1, "write WAV",
           preexec_fn=limit_output_size)
    expect(executable, environment, ["--device", "test_capture", outputs / "missing" / "out.wav"],
           1, "create temporary WAV")
    reserved = outputs / "reserved.wav"
    sentinel = b"existing output"
    reserved.write_bytes(sentinel)
    expect(executable, environment, ["--device", "test_capture", reserved], 1, "destination already exists")
    if reserved.read_bytes() != sentinel or set(outputs.iterdir()) != {recorded, reserved}:
        raise RuntimeError("a failed capture modified an existing file or left output behind")
    print("PASS usage, missing PCM, PCM read error, WAV write error, invalid destination, no-clobber, and cleanup")


if __name__ == "__main__":
    with TemporaryDirectory(prefix="ai-robot-f002-") as temporary:
        verify_capture(sys.argv[1], Path(temporary))
