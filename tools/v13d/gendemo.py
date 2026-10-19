"""Write a 1.3D demo whose input is a seeded random walk that fires, jumps, opens
and switches weapons. Both the DOS executable and our 1.3D build can play it, so
it can test code the original demos never reach. Blocks are stored raw
(strtot 0), which the 1996 LZW reader accepts.

usage: gendemo.py <out.dmo> <volume> <level> <skill> <tics> <seed> [weapon 1-10]

With a weapon number, the walk keeps selecting that weapon."""
import struct, sys, random
out, vol, lev, skill, tics, seed = sys.argv[1], *map(int, sys.argv[2:7])
force = int(sys.argv[7]) if len(sys.argv) > 7 else 0   # weapon number to keep selecting (1-10)
rng = random.Random(seed)
recs = []
fv = sv = av = 0; bits = 0; left = 0
for t in range(tics):
    if left == 0:
        left = rng.randint(15, 90)
        fv = rng.choice([90, 90, 90, 0, -60]); sv = rng.choice([0, 0, 40, -40])
        av = rng.choice([0, 0, 0, 20, -20, 60, -60]); fire = rng.random() < 0.5
        weap = rng.choice([0]*8 + [2, 3, 4, 5, 6, 7, 9, 10])
    left -= 1
    bits = (1 << 2) if fire and rng.random() < 0.7 else 0
    if force and t % 40 == 0: bits |= (force & 15) << 8
    elif weap and left % 30 == 0: bits |= (weap & 15) << 8
    if rng.random() < 0.02: bits |= 1          # jump
    if rng.random() < 0.03: bits |= 1 << 29     # open/use
    recs.append(struct.pack("<bbhhI", av, 0, fv, sv, bits))
hdr = struct.pack("<iBBBBhhiii", tics, vol, lev, skill, 0, 1, 0, 0, 0, 0)
data = bytearray(hdr)
for b in range(0, tics, 2520):                    # one dfwrite per 2520-record batch
    batch = recs[b:b+2520]
    raw = bytearray(batch[0])
    for i in range(1, len(batch)):
        raw += bytes((batch[i][j] - batch[i-1][j]) & 255 for j in range(10))
    for c in range(0, len(raw), 16380):         # dfwrite's block size
        chunk = raw[c:c+16380]
        data += struct.pack("<hHH", len(chunk) + 4, len(chunk), 0) + chunk
open(out, "wb").write(data)
