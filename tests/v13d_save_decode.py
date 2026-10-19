#!/usr/bin/env python3
"""Independent decoder for Duke Nukem 3D 1.3D (DOS) saves.

Written from the DOS file layout (docs/v13d/reference.md, "Save file"), not
from the game's save code. 1.3D compresses each block with the 1996 LZW
variant (ledger E-012), which this file implements itself.

  v13d_save_decode.py compare <dos save> <our save>
      decompress both files block by block. Prints one line per block that
      differs and exits 1 if the block layouts differ or if any block other
      than the ones our saves legitimately change differs (see compare()).

  v13d_save_decode.py script <save> <out>
      write the compiled CON script the save holds, one "<name> <index> <value>"
      line per word: "script" for script[0..20000) and "actor" for each
      non-zero actorscrptr[0..4096) entry. Script addresses are byte offsets
      from script[0], which is how 1.3D stores them.
"""
import struct
import sys

PLAYER_SIZE = 0x552
BYTEVERSIONS = (103, 14)  # full, shareware


def uncompress(b):
    """cache1d.c uncompress() as of engine revision 19960419."""
    uleng, strtot = struct.unpack_from('<HH', b, 0)
    if strtot == 0:
        return b[4:4 + uleng]
    b = b + b'\0' * 8
    prefix = list(range(256)) + [0] * (strtot + 2)
    suffix = list(range(256)) + [0] * (strtot + 2)
    out = bytearray()
    currstr, inbyte, bitcnt, numbits, oneup = 256, 4, 0, 9, 512
    while True:
        dat = (struct.unpack_from('<I', b, inbyte)[0] >> bitcnt) & (oneup - 1)
        bitcnt += numbits
        inbyte += bitcnt >> 3
        bitcnt &= 7
        suffix[currstr] = dat
        tail = []
        while dat >= 256:
            tail.append(prefix[dat])
            dat = suffix[dat]
        out.append(dat)
        out.extend(reversed(tail))
        prefix[currstr - 1] = dat
        prefix[currstr] = dat
        currstr += 1
        if currstr >= oneup:
            numbits += 1
            oneup <<= 1
        if currstr >= strtot:
            break
    return bytes(out[:uleng])


class Reader:
    """kdfread(): blocks of deltas between consecutive elements."""

    def __init__(self, data):
        self.data, self.pos = data, 0

    def block(self):
        if self.pos + 2 > len(self.data):
            raise ValueError('save ends early')
        n, = struct.unpack_from('<h', self.data, self.pos)
        self.pos += 2
        chunk = self.data[self.pos:self.pos + n]
        self.pos += n
        return uncompress(chunk)

    def read(self, size, count):
        buf = self.block()
        prev = bytearray(buf[:size])
        out, k = bytearray(prev), size
        for _ in range(1, count):
            if k >= len(buf):
                buf, k = self.block(), 0
            prev = bytearray((prev[j] + buf[k + j]) & 255 for j in range(size))
            out += prev
            k += size
        return bytes(out)


def read_script(path):
    r = Reader(open(path, 'rb').read())
    version, = struct.unpack('<i', r.read(4, 1))
    if version not in BYTEVERSIONS:
        raise ValueError('not a 1.3D save (BYTEVERSION %d)' % version)
    # Everything saveplayer writes before the script, in order.
    for size, count in ((19, 1), (4, 1), (4, 1), (4, 1), (160, 100),
                        (2, 1), (32, 8192), (2, 1), (40, 1024), (44, 4096),
                        (2, 1025), (2, 4096), (2, 4096),
                        (2, 1025), (2, 4096), (2, 4096),
                        (2, 1), (12, 256), (PLAYER_SIZE, 16), (16, 16),
                        (2, 1), (3072, 1), (4, 2048), (4, 2048),
                        (2, 1), (2, 64), (2, 1), (2, 64), (2, 64), (1, 128)):
        r.read(size, count)
    r.read(1, 20000)  # which words were addresses; they're offsets either way
    script = struct.unpack('<20000i', r.read(4, 20000))
    actors = struct.unpack('<4096i', r.read(4, 4096))
    return script, actors


def blocks(path):
    """Every block's decompressed bytes, in file order."""
    data, pos, out = open(path, 'rb').read(), 0, []
    while pos + 2 <= len(data):
        n, = struct.unpack_from('<h', data, pos)
        out.append(uncompress(data[pos + 2:pos + 2 + n]))
        pos += 2 + n
    if pos != len(data):
        raise ValueError('%s: a block runs past the end' % path)
    return out


# Blocks our saves change, by index in a single-player save: the script and
# hittype pointer bitmaps (we keep script values as offsets, so ours are all
# zero), and lockclock (loadplayer resets the clock, as DOS does).
ALLOWED = {60: 'script bitmap', 61: 'script bitmap', 68: 'hittype bitmap', 78: 'lockclock'}
PALETTE = 0x541     # each player's DOS palette pointer; we write zeros
GM = 0x516          # loading clears MODE_MENU (1) from gm, as DOS's loadplayer does


def compare(dos, ours):
    a, b = blocks(dos), blocks(ours)
    if [len(x) for x in a] != [len(x) for x in b]:
        print('block layouts differ: %d blocks %s vs %d blocks %s'
              % (len(a), [len(x) for x in a], len(b), [len(x) for x in b]))
        return 1
    # animateptr: entries past animatecnt are stale in DOS saves and never
    # read, and we write zeros there.
    n = 4 * struct.unpack('<i', a[81])[0]
    a[83], b[83] = a[83][:n], b[83][:n]
    # ps[0..11] and ps[12..15]: kdfread stores each player as a difference
    # from the one before, so undo that before masking.
    for x in (a, b):
        ps = bytearray(x[47] + x[48])
        for p in range(PLAYER_SIZE, len(ps), PLAYER_SIZE):
            for j in range(PLAYER_SIZE):
                ps[p + j] = (ps[p + j] + ps[p - PLAYER_SIZE + j]) & 255
        for p in range(0, len(ps), PLAYER_SIZE):
            ps[p + PALETTE:p + PALETTE + 4] = bytes(4)
            ps[p + GM] &= 0xfe
        x[47], x[48] = bytes(ps[:len(x[47])]), bytes(ps[len(x[47]):])
    bad = 0
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y:
            print('block %d differs (%s)' % (i, ALLOWED.get(i, 'not allowed')))
            bad += i not in ALLOWED
    return 1 if bad else 0


def main():
    if len(sys.argv) == 4 and sys.argv[1] == 'compare':
        return compare(sys.argv[2], sys.argv[3])
    if len(sys.argv) == 4 and sys.argv[1] == 'script':
        script, actors = read_script(sys.argv[2])
        with open(sys.argv[3], 'w') as f:
            for i, v in enumerate(script):
                f.write('script %d %d\n' % (i, v))
            for i, v in enumerate(actors):
                if v:
                    f.write('actor %d %d\n' % (i, v))
        return 0
    sys.stderr.write(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main())
