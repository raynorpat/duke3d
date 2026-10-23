#!/usr/bin/env python3
"""Independent decoder for Duke Nukem 3D 1.3D, 1.1, 1.0 and 0.99 (DOS) saves.

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
      BYTEVERSION 12, or the 19-byte save name in 1.0/0.99 (no version). Their player-block
      sizes distinguish 1.0 from 0.99.

  v13d_save_decode.py truncate <save> <out> <n>
      write the save without its last n bytes.

  v13d_save_decode.py badaddr10 <1.0 save> <out>
      write the 1.0 save with one actor's hittype temp_data[4] set to the
      script address just past the end of the script (re-encoding hittype[]
      as stored blocks), which a loader must refuse.
"""
import struct
import sys

PLAYER_SIZE = 0x552
BYTEVERSIONS = (103, 14)  # full, shareware
BYTEVERSION_11 = 12
PLAYER_SIZE_11 = 0x34e    # 1.1 and 1.0
PLAYER_SIZE_099 = 0x33f


def uncompress(b):
    """filesystem.c uncompress() as of engine revision 19960419."""
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
    raw hittype[] before the animated walls. The beta has the same prefix,
    a smaller player block, and no show2dsector block."""
    for size, count in ((19, 1), (4, 1), (4, 1), (4, 1), (160, 100)):
        r.read(size, count)
    numwalls, = struct.unpack('<h', r.read(2, 1))
    r.read(32, numwalls)
    numsectors, = struct.unpack('<h', r.read(2, 1))
    r.read(40, numsectors)
    for size, count in ((44, 4096),
                        (2, 1025), (2, 4096), (2, 4096),
                        (2, 1025), (2, 4096), (2, 4096),
                        (2, 1), (12, 256)):
        r.read(size, count)
    pos = r.pos
    pbytes = len(r.block())
    r.pos = pos
    if pbytes == PLAYER_SIZE_099 * 8:
        psize = PLAYER_SIZE_099
    elif pbytes == PLAYER_SIZE_11 * 8:
        psize = PLAYER_SIZE_11
    else:
        raise ValueError('not a 0.99 or 1.0 player block (%d bytes)' % pbytes)
    for size, count in ((psize, 8), (16, 8), (71, 2048),
                        (2, 1), (3072, 1), (4, 2048), (4, 2048),
                        (2, 1), (2, 64), (2, 1), (2, 64), (2, 64)):
        r.read(size, count)
    if psize == PLAYER_SIZE_11:
        r.read(1, 128)
    r.read(1, 10240)
    script = struct.unpack('<10240i', r.read(4, 10240))
    # Every entry is stored minus the script's run-time address, so an empty
    # one comes out negative (-0x2d5f08 in the DOSBox saves).
    actors = tuple(max(v, 0) for v in struct.unpack('<4096i', r.read(4, 4096)))
    return script, actors


LZWSIZE = 16384


def dfsizes(size, count):
    """The uncompressed size of each block dfwrite(size, count) writes."""
    if size > LZWSIZE:
        size, count = 1, size * count
    out, k = [], 0
    for i in range(count):
        k += size
        if k > LZWSIZE - size or i == count - 1:
            out.append(k)
            k = 0
    return out


def calls_10(bl):
    """The 1.0 saveplayer's dfwrite calls (0x388d0) as (name, first block,
    block count), given the save's decompressed blocks."""
    out, i = [], 0

    def call(name, size, count):
        nonlocal i
        n = len(dfsizes(size, count))
        out.append((name, i, n))
        i += n

    for size, count in ((19, 1), (4, 1), (4, 1), (4, 1), (160, 100)):
        call('', size, count)
    call('numwalls', 2, 1)
    call('wall', 32, struct.unpack('<h', bl[i - 1])[0])
    call('numsectors', 2, 1)
    call('sector', 40, struct.unpack('<h', bl[i - 1])[0])
    call('sprite', 44, 4096)
    for size, count in ((2, 1025), (2, 4096), (2, 4096),
                        (2, 1025), (2, 4096), (2, 4096), (2, 1), (12, 256)):
        call('', size, count)
    beta = len(bl[i]) == 8 * 0x33f
    call('ps', 0x33f if beta else PLAYER_SIZE_11, 8)
    call('po', 16, 8)
    call('hittype', 71, 2048)
    for size, count in ((2, 1), (3072, 1), (4, 2048), (4, 2048),
                        (2, 1), (2, 64), (2, 1), (2, 64), (2, 64)):
        call('', size, count)
    if not beta:
        call('', 1, 128)
    call('script bitmap', 1, 10240)
    call('script', 4, 10240)
    call('actorscrptr', 4, 4096)
    call('lockclock', 4, 1)
    for size, count in ((2, 1), (2, 256)):
        call('', size, count)
    call('animatecnt', 4, 1)
    call('', 2, 64)
    call('animateptr', 4, 64)
    for size, count in ((4, 64), (4, 64), (1, 1), (2, 1)):
        call('', size, count)
    for size, count in (((2, 1),) if beta else ((4, 1), (4, 1))):
        call('', size, count)
    return out


def layout_10(bl):
    c = {name: (first, n) for name, first, n in calls_10(bl) if name}
    beta = len(bl[c['ps'][0]]) == 8 * 0x33f
    return dict(allowed={c['script bitmap'][0]: 'script bitmap',
                         c['lockclock'][0]: 'lockclock'},
                animatecnt=c['animatecnt'][0], animateptr=c['animateptr'][0],
                ps=tuple(range(c['ps'][0], c['ps'][0] + c['ps'][1])),
                psize=0x33f if beta else PLAYER_SIZE_11,
                palette=0x308 if beta else 0x33a, gm=0x2dc if beta else 0x30c, gmmask=0,
                redraw=(0x322, 0x326) if beta else None)


def chunks(path):
    """Every block's stored bytes (length word included), in file order."""
    data, pos, out = open(path, 'rb').read(), 0, []
    while pos + 2 <= len(data):
        n, = struct.unpack_from('<h', data, pos)
        out.append(data[pos:pos + 2 + n])
        pos += 2 + n
    return out


def badaddr_10(src, dst):
    bl, ch = blocks(src), chunks(src)
    c = {name: (first, n) for name, first, n in calls_10(bl) if name}

    def call(name, size):
        first, n = c[name]
        r = Reader(b''.join(ch[first:first + n]))
        return r.read(size, len(b''.join(bl[first:first + n])) // size)

    spr = call('sprite', 44)
    actors = struct.unpack('<4096i', call('actorscrptr', 4))
    base = -min(actors)
    ht = bytearray(call('hittype', 71))
    for i in range(2048):
        pic, = struct.unpack_from('<h', spr, i * 44 + 14)
        o = i * 71 + 47 + 4 * 4
        t4, = struct.unpack_from('<i', ht, o)
        if 0 <= pic < 4096 and actors[pic] >= 0 and base <= t4 < base + 0xa000:
            struct.pack_into('<i', ht, o, base + 0xa000)
            break
    else:
        raise ValueError('%s: no actor with a script address' % src)
    # dfwrite's delta coding, in stored (uncompressed) blocks.
    delta = bytearray(ht[:71])
    for j in range(71, len(ht)):
        delta.append((ht[j] - ht[j - 71]) & 255)
    new, k = [], 0
    for size in dfsizes(71, 2048):
        new.append(struct.pack('<hHH', size + 4, size, 0) + bytes(delta[k:k + size]))
        k += size
    first, n = c['hittype']
    ch[first:first + n] = new
    open(dst, 'wb').write(b''.join(ch))


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
    if len(a[0]) == 19:     # 1.0: no BYTEVERSION, the save name comes first
        L = layout_10(a)
    else:
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
        # Native loadplayer/vscrn request a redraw for the active player.
        # Inactive players' saved display flags must still match exactly.
        if L.get('redraw'):
            first, last = L['redraw']
            ps[first:last] = bytes(last - first)
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
    if len(sys.argv) == 5 and sys.argv[1] == 'truncate':
        data = open(sys.argv[2], 'rb').read()
        open(sys.argv[3], 'wb').write(data[:len(data) - int(sys.argv[4])])
        return 0
    if len(sys.argv) == 4 and sys.argv[1] == 'badaddr10':
        badaddr_10(sys.argv[2], sys.argv[3])
        return 0
    sys.stderr.write(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main())
