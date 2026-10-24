"""Base GRPs must fit the legacy loader's signed offset arithmetic."""
import argparse
import os
from pathlib import Path
import struct
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("identify")
parser.add_argument("game")
parser.add_argument("work")
a = parser.parse_args()
Path(a.work).mkdir(parents=True, exist_ok=True)

# Individual sizes, cumulative offsets and directory-prefix seeks are separate risks.
cases = [
    ("member", [0x80000000], None),
    ("cumulative", [0x40000000, 0x40000000], None),
    ("seek_prefix", [0x7FFFFFF0], None),
    ("directory", [], 134217727),
]
for name, lengths, count in cases:
    with tempfile.TemporaryDirectory(prefix=name + "-", dir=a.work) as scratch:
        root = Path(scratch)
        grp = root / "duke3d.grp"
        with grp.open("wb") as f:
            f.write(b"KenSilverman" + struct.pack("<I", len(lengths) if count is None else count))
            for i, length in enumerate(lengths):
                f.write(struct.pack("<12sI", b"GAME.CON" if not i else b"OTHER.DAT", length))
            # Sparse data proves the old inspector accepted a structurally complete
            # archive. On Windows test the same metadata rejection without a huge allocation.
            if count is None and os.name != "nt":
                f.truncate(16 + len(lengths) * 16 + sum(lengths))
        commands = [[a.identify, "--identify", str(grp)], [a.game, "-gameversion", "1.5"]]
        for command in commands:
            result = subprocess.run(command, cwd=root, capture_output=True, text=True,
                                    timeout=45, env=dict(os.environ, SDL_VIDEODRIVER="dummy",
                                                       SDL_AUDIODRIVER="dummy", HOME=scratch,
                                                       USERPROFILE=scratch, DUKE3D_SAVE_DIR=scratch))
            output = result.stdout + result.stderr
            assert result.returncode != 0 and "GRP exceeds legacy 32-bit limits" in output, (name, command, result.returncode, output)
            assert "Selected game profile:" not in output, output
print("Oversized member, cumulative extent, seek-prefix and directory GRPs refused, including forced startup.")
