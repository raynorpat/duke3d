#!/usr/bin/env python3
"""Independent decoder for Duke Nukem 3D 1.3D, 1.1 and 1.0 (DOS) saves.

Written from the DOS file layouts (docs/v13d/reference.md and
docs/v10-v11/reference.md, "Save file"), not from the game's save code. All
three compress each block with the 1996 LZW variant (ledger E-012), which
this file implements itself.

  v13d_save_decode.py compare <dos save> <our save>
      decompress both files block by block. Prints one line per block that
      differs and exits 1 if the block layouts differ or if any block other
      than the ones our saves legitimately change differs (see compare()).

  v13d_save_decode.py script <save> <out>
      write the compiled CON script the save holds, one "<name> <index> <value>"
      line per word: "script" for script[0..20000) (1.1 and 1.0: 10240) and
      "actor" for each non-zero actorscrptr[0..4096) entry. Script addresses
      are byte offsets from script[0], which is how all three store them.
      The save's layout is told from its first block: a 1.3D BYTEVERSION, 1.1's
      BYTEVERSION 12, or 1.0's 19-byte save name (1.0 writes no version).
"""
import struct
import sys

PLAYER_SIZE = 0x552
BYTEVERSIONS = (103, 14)  # full, shareware
BYTEVERSION_11 = 12
PLAYER_SIZE_11 = 0x34e    # 1.1 and 1.0


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
    first = r.block()
    r.pos = 0
    if len(first) == 19:
        return read_script_10(r)
    version, = struct.unpack('<i', r.read(4, 1))
    if version == BYTEVERSION_11:
        psize, nplayers, nscript = PLAYER_SIZE_11, 8, 10240
    elif version in BYTEVERSIONS:
        psize, nplayers, nscript = PLAYER_SIZE, 16, 20000
    else:
        raise ValueError('not a 1.3D or 1.1 save (BYTEVERSION %d)' % version)
    # Everything saveplayer writes before the script, in order.
    for size, count in ((19, 1), (4, 1), (4, 1), (4, 1), (160, 100),
                        (2, 1), (32, 8192), (2, 1), (40, 1024), (44, 4096),
                        (2, 1025), (2, 4096), (2, 4096),
                        (2, 1025), (2, 4096), (2, 4096),
                        (2, 1), (12, 256), (psize, nplayers), (16, nplayers),
                        (2, 1), (3072, 1), (4, 2048), (4, 2048),
                        (2, 1), (2, 64), (2, 1), (2, 64), (2, 64), (1, 128)):
        r.read(size, count)
    r.read(1, nscript)  # which words were addresses; they're offsets either way
    script = struct.unpack('<%di' % nscript, r.read(4, nscript))
    actors = struct.unpack('<4096i', r.read(4, 4096))
    return script, actors


def read_script_10(r):
    """1.0: no BYTEVERSION, only numwalls walls and numsectors sectors, and a
    raw hittype[] before the animated walls."""
    for size, count in ((19, 1), (4, 1), (4, 1), (4, 1), (160, 100)):
        r.read(size, count)
    numwalls, = struct.unpack('<h', r.read(2, 1))
    r.read(32, numwalls)
    numsectors, = struct.unpack('<h', r.read(2, 1))
    r.read(40, numsectors)
    for size, count in ((44, 4096),
                        (2, 1025), (2, 4096), (2, 4096),
                        (2, 1025), (2, 4096), (2, 4096),
                        (2, 1), (12, 256), (PLAYER_SIZE_11, 8), (16, 8),
                        (71, 2048),
                        (2, 1), (3072, 1), (4, 2048), (4, 2048),
                        (2, 1), (2, 64), (2, 1), (2, 64), (2, 64), (1, 128)):
        r.read(size, count)
    r.read(1, 10240)
    script = struct.unpack('<10240i', r.read(4, 10240))
    # Every entry is stored minus the script's run-time address, so an empty
    # one comes out negative (-0x2d5f08 in the DOSBox saves).
    actors = tuple(max(v, 0) for v in struct.unpack('<4096i', r.read(4, 4096)))
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


# Per layout, by index in a single-player save: the blocks our saves change
# (the script and hittype pointer bitmaps, which we keep zeroed because our
# script values are offsets already, and lockclock, which loadplayer resets
# as DOS does), the animatecnt and animateptr blocks, the ps[] blocks, the
# player_struct size, and each player's DOS palette pointer (we write zeros)
# and gm, with the bits our saves may change: loading clears MODE_MENU (1), as
# 1.3D's loadplayer does; 1.1's loadplayer overwrites gm, and its saves hold 0.
LAYOUTS = {
    '1.3D': dict(allowed={60: 'script bitmap', 61: 'script bitmap',
                          68: 'hittype bitmap', 78: 'lockclock'},
                 animatecnt=81, animateptr=83, ps=(47, 48),
                 psize=PLAYER_SIZE, palette=0x541, gm=0x516, gmmask=0xfe),
    '1.1': dict(allowed={59: 'script bitmap', 64: 'hittype bitmap', 74: 'lockclock'},
                animatecnt=77, animateptr=79, ps=(47,),
                psize=PLAYER_SIZE_11, palette=0x33b, gm=0x30c, gmmask=0),
}


def compare(dos, ours):
    a, b = blocks(dos), blocks(ours)
    if [len(x) for x in a] != [len(x) for x in b]:
        print('block layouts differ: %d blocks %s vs %d blocks %s'
              % (len(a), [len(x) for x in a], len(b), [len(x) for x in b]))
        return 1
    version, = struct.unpack('<i', a[0])
    L = LAYOUTS['1.1' if version == BYTEVERSION_11 else '1.3D']
    # animateptr: entries past animatecnt are stale in DOS saves and never
    # read, and we write zeros there.
    n = 4 * struct.unpack('<i', a[L['animatecnt']])[0]
    i = L['animateptr']
    a[i], b[i] = a[i][:n], b[i][:n]
    # ps[]: kdfread stores each player as a difference from the one before,
    # so undo that before masking.
    size = L['psize']
    for x in (a, b):
        ps = bytearray(b''.join(x[k] for k in L['ps']))
        for p in range(size, len(ps), size):
            for j in range(size):
                ps[p + j] = (ps[p + j] + ps[p - size + j]) & 255
        for p in range(0, len(ps), size):
            ps[p + L['palette']:p + L['palette'] + 4] = bytes(4)
            ps[p + L['gm']] &= L['gmmask']
        o = 0
        for k in L['ps']:
            x[k], o = bytes(ps[o:o + len(x[k])]), o + len(x[k])
    bad = 0
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y:
            print('block %d differs (%s)' % (i, L['allowed'].get(i, 'not allowed')))
            bad += i not in L['allowed']
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
