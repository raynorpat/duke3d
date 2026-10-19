"""Copy a GRP with one map's player start moved onto a sprite, so a generated demo
(gendemo.py) starts next to the enemy or pickup under test. The map keeps its
length; only the header's start position, angle and sector change.

usage: startnear.py <in.grp> <out.grp> <MAP name> <picnum> [which]

The start goes onto the <which>-th sprite of <picnum> (default the first), at its
sector's floor height."""
import struct, sys
src, dst, mapname, pic = sys.argv[1], sys.argv[2], sys.argv[3].upper(), int(sys.argv[4])
which = int(sys.argv[5]) if len(sys.argv) > 5 else 0
d = bytearray(open(src,"rb").read())
n = struct.unpack_from("<i", d, 12)[0]; off = 16+16*n
for k in range(n):
    nm = d[16+16*k:28+16*k].rstrip(b"\0").decode(); sz = struct.unpack_from("<i", d, 28+16*k)[0]
    if nm == mapname: break
    off += sz
else: raise SystemExit("no map " + mapname)
m = off; p = m+4+4*3+2+2
ns = struct.unpack_from("<h", d, p)[0]; sec = p+2; p += 2+40*ns
nw = struct.unpack_from("<h", d, p)[0]; p += 2+32*nw
nsp = struct.unpack_from("<h", d, p)[0]; p += 2
hits = [i for i in range(nsp) if struct.unpack_from("<h", d, p+44*i+14)[0] == pic]
s = p+44*hits[which]
x, y, z = struct.unpack_from("<iii", d, s); sect = struct.unpack_from("<h", d, s+24)[0]
floorz = struct.unpack_from("<i", d, sec+40*sect+8)[0]   # sectortype.floorz
struct.pack_into("<iiihh", d, m+4, x, y, floorz-(38<<8), 0, sect)
open(dst, "wb").write(d)
print(f"{mapname}: start at sprite {hits[which]} ({x},{y}) sector {sect}, {len(hits)} of picnum {pic}")
