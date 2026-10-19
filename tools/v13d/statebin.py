"""Print reference values from a STATE.BIN written by a mkcapture.py-patched EXE.
usage: statebin.py <STATE.BIN> <id>   (id like full_1)  -> CMake set() lines."""
import struct, sys
d = open(sys.argv[1], 'rb').read(); ident = sys.argv[2].upper()
reccnt, seed = struct.unpack_from('<iI', d, 0)
pl = []
for i in range(4096):
    a = 8 + i*44
    x, y, z = struct.unpack_from('<iii', d, a)
    picnum = struct.unpack_from('<h', d, a + 0xe)[0]
    statnum, ang = struct.unpack_from('<hh', d, a + 0x1a)
    extra = struct.unpack_from('<h', d, a + 0x2a)[0]
    if picnum == 1405 and statnum == 10: pl.append((x, y, z, ang, extra))
if len(pl) != 1: sys.exit(f'expected one player sprite, got {pl}')
if reccnt != 0: sys.exit(f'reccnt is {reccnt}, not 0: the loop overran or stopped early; capture again')
x, y, z, ang, hp = pl[0]
for k, v in (('X', x), ('Y', y), ('Z', z), ('ANG', ang), ('HEALTH', hp), ('RANDOMSEED', seed)):
    print(f'set(V13D_DEMO_{ident}_{k} {v})')
