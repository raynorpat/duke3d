"""Extract only the three stock CON files for an isolated boot fixture."""
from pathlib import Path
import struct
import sys

path, target = Path(sys.argv[1]), Path(sys.argv[2])
with path.open('rb') as f:
    header = f.read(16)
    if len(header) != 16 or header[:12] != b'KenSilverman':
        raise SystemExit('invalid GRP')
    count = struct.unpack_from('<I', header, 12)[0]
    if count > (path.stat().st_size - 16) // 16:
        raise SystemExit('truncated GRP directory')
    entries = [f.read(16) for _ in range(count)]
    offset, found = 16 + count*16, set()
    for entry in entries:
        name = entry[:12].split(b'\0')[0].decode('ascii').upper()
        size = struct.unpack_from('<I', entry, 12)[0]
        if name in ('GAME.CON', 'DEFS.CON', 'USER.CON'):
            f.seek(offset)
            data = f.read(size)
            if len(data) != size:
                raise SystemExit('truncated CON member')
            (target / name).write_bytes(data)
            found.add(name)
        offset += size
    if len(found) != 3:
        raise SystemExit('GRP does not contain the three default CON files')
