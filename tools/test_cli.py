#!/usr/bin/env python3
"""Check the real PCM CLI, endian/length contract and refusal to overwrite."""
import math
import pathlib
import struct
import subprocess
import sys
import tempfile

executable = pathlib.Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix="rnnoise-cli-") as directory:
    root = pathlib.Path(directory)
    source = root / "input.s16le"
    output = root / "output.s16le"
    samples = [int(9000 * math.sin(i * math.pi * 2 * 137 / 48000)) for i in range(480 * 100)]
    source.write_bytes(struct.pack("<" + "h" * len(samples), *samples))
    subprocess.run([str(executable), str(source), str(output)], check=True)
    assert output.stat().st_size == source.stat().st_size
    assert output.read_bytes() != source.read_bytes()
    previous = output.read_bytes()
    assert subprocess.run([str(executable), str(source), str(output)]).returncode != 0
    assert output.read_bytes() == previous
    assert subprocess.run([str(executable), str(source), str(source)]).returncode != 0
    source.write_bytes(bytes(961))
    partial = root / "partial.s16le"
    assert subprocess.run([str(executable), str(source), str(partial)]).returncode != 0
    assert partial.stat().st_size == 960
    source.write_bytes(bytes(480 * 2 * 5))
    silence = root / "silence.s16le"
    subprocess.run([str(executable), str(source), str(silence)], check=True)
    assert silence.read_bytes() == source.read_bytes()
print("PASS: actual CLI processing, little-endian PCM size, silence, incomplete frames, overwrite protection")
