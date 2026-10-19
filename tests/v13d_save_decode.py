#!/usr/bin/env python3
"""Independent decoder for Duke Nukem 3D 1.3D (DOS) saves.

Written from the DOS file layout (docs/v13d/reference.md, "Save file"), not
from the game's save code. 1.3D compresses each block with the 1996 LZW
variant (ledger E-012), which this file implements itself.

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


def main():
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
