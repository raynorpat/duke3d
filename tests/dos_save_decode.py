#!/usr/bin/env python3
"""Independent decoder for Duke Nukem 3D 1.5 (DOS, BYTEVERSION 117) saves.

Written from the DOS file layout, not from the game's save code, so the
tests can hold the two against each other.

  dos_save_decode.py check <save>      parse the whole file; exit 1 on error
  dos_save_decode.py state <save>      print the SAVESTATE line -loadverify prints
  dos_save_decode.py compare <a> <b>   compare two saves field by field
  dos_save_decode.py fixture oldlayout <in> <out>
                                       copy with a 14720-byte player block
  dos_save_decode.py fixture version <in> <out>
                                       copy with BYTEVERSION 103
"""
import struct
import sys

LZWSIZE = 16384
MAXWALLS, MAXSECTORS, MAXSPRITES, MAXSTATUS = 8192, 1024, 4096, 1024
MAXPLAYERS, MAXCYCLERS, MAXANIMWALLS = 16, 256, 512
MAXSCRIPTSIZE, DOS_MAXTILES, MAXPSKYTILES, MAXANIMATES = 20460, 6144, 256, 64
BYTEVERSION = 117


def fields(code, names):
    out = []
    for n in names.split():
        name, _, count = n.partition('*')
        out.append((name, code, int(count or 1)))
    return out


# struct player_struct, packed to 1 byte, in declaration order.
PLAYER = (
    fields('i', 'zoom exitx exity loogiex*64 loogiey*64 numloogs loogcnt '
                'posx posy posz horiz ohoriz ohorizoff invdisptime '
                'bobposx bobposy oposx oposy oposz pyoff opyoff '
                'posxv posyv poszv last_pissed_time truefz truecz '
                'player_par visibility bobcounter weapon_sway '
                'pals_time randomflamex crack_time aim_mode')
    + fields('h', 'ang oang angvel cursectnum look_ang last_extra subweapon '
                  'ammo_amount*12 wackedbyactor frag fraggedself '
                  'curr_weapon last_weapon tipincs horizoff wantweaponfire '
                  'holoduke_amount newowner hurt_delay hbomb_hold_delay '
                  'jumping_counter airleft knee_incs access_incs '
                  'fta ftq access_wallnum access_spritenum '
                  'kickback_pic got_access weapon_ang firstaid_amount '
                  'somethingonplayer on_crane i one_parallax_sectnum '
                  'over_shoulder_on random_club_frame fist_incs '
                  'one_eighty_count cheat_phase '
                  'dummyplayersprite extra_extra8 quick_kick '
                  'heat_amount actorsqu timebeforeexit customexitsound '
                  'weaprecs*16 weapreccnt interface_toggle_flag '
                  'rotscrnang dead_flag show_empty_weapon '
                  'scuba_amount jetpack_amount steroids_amount shield_amount '
                  'holoduke_on pycount weapon_pos frag_ps '
                  'transporter_hold last_full_weapon footprintshade boot_amount')
    + fields('i', 'scream_voice')
    + fields('B', 'gm on_warping_sector footprintcount '
                  'hbomb_on jumping_toggle rapid_fire_hold on_ground '
                  'name*32 inven_icon buttonpalette '
                  'jetpack_on spritebridge lastrandomspot '
                  'scuba_on footprintpal heat_on '
                  'holster_weapon falling_counter gotweapon*12 refresh_inventory')
    + fields('I', 'palette')
    + fields('B', 'toggle_key_flag knuckle_incs '
                  'walking_snd_toggle palookup hard_landing '
                  'max_secret_rooms secret_rooms pals*3 '
                  'max_actors_killed actors_killed return_to_center'))

SPRITE = (fields('i', 'x y z') + fields('h', 'cstat picnum') + fields('b', 'shade')
          + fields('B', 'pal clipdist filler xrepeat yrepeat')
          + fields('b', 'xoffset yoffset')
          + fields('h', 'sectnum statnum ang owner xvel yvel zvel lotag hitag extra'))


def layout(flds):
    offs, o = {}, 0
    for name, code, count in flds:
        offs[name] = (o, code)
        o += struct.calcsize('<' + code) * count
    return offs, o


PLAYER_OFFS, PLAYER_SIZE = layout(PLAYER)
SPRITE_OFFS, SPRITE_SIZE = layout(SPRITE)
assert PLAYER_SIZE == 911 and SPRITE_SIZE == 44
assert PLAYER_OFFS['posx'][0] == 532 and PLAYER_OFFS['shield_amount'][0] == 810
assert PLAYER_OFFS['gm'][0] == 832 and PLAYER_OFFS['palette'][0] == 894


def get(offs, rec, name, index=0):
    o, code = offs[name]
    return struct.unpack_from('<' + code, rec, o + index * struct.calcsize('<' + code))[0]


# Every dfwrite call the DOS saveplayer makes, in order: (name, element
# size, count). spriteq's count is the spriteqamount value just before it.
CALLS = [
    ('version', 4, 1), ('multimode', 4, 1), ('name', 19, 1),
    ('volume', 4, 1), ('level', 4, 1), ('skill', 4, 1), ('thumbnail', 160, 100),
    ('numwalls', 2, 1), ('wall', 32, MAXWALLS),
    ('numsectors', 2, 1), ('sector', 40, MAXSECTORS),
    ('sprite', 44, MAXSPRITES),
    ('headspritesect', 2, MAXSECTORS + 1), ('prevspritesect', 2, MAXSPRITES),
    ('nextspritesect', 2, MAXSPRITES), ('headspritestat', 2, MAXSTATUS + 1),
    ('prevspritestat', 2, MAXSPRITES), ('nextspritestat', 2, MAXSPRITES),
    ('numcyclers', 2, 1), ('cyclers', 12, MAXCYCLERS),
    ('ps', 911 * MAXPLAYERS, 1), ('po', 16 * MAXPLAYERS, 1),
    ('numanimwalls', 2, 1), ('animwall', 6 * MAXANIMWALLS, 1),
    ('msx', 4, 2048), ('msy', 4, 2048),
    ('spriteqloc', 2, 1), ('spriteqamount', 2, 1), ('spriteq', 2, None),
    ('mirrorcnt', 2, 1), ('mirrorwall', 2, 64), ('mirrorsector', 2, 64),
    ('show2dsector', 1, MAXSECTORS >> 3), ('actortype', 1, DOS_MAXTILES),
    ('boardfilename', 128, 1),
    ('numclouds', 2, 1), ('clouds', 256, 1), ('cloudx', 256, 1), ('cloudy', 256, 1),
    ('scriptptrs', 1, MAXSCRIPTSIZE), ('script', 4, MAXSCRIPTSIZE),
    ('actorscrptr', 4, DOS_MAXTILES),
    ('hitptrs', 1, MAXSPRITES), ('hittype', 71, MAXSPRITES),
    ('lockclock', 4, 1), ('pskybits', 2, 1), ('pskyoff', 2, MAXPSKYTILES),
    ('animatecnt', 4, 1), ('animatesect', 2, MAXANIMATES),
    ('animateptr', 4, MAXANIMATES), ('animategoal', 4, MAXANIMATES),
    ('animatevel', 4, MAXANIMATES),
    ('earthquaketime', 1, 1), ('from_bonus', 2, 1), ('secretlevel', 2, 1),
    ('respawn_monsters', 4, 1), ('respawn_items', 4, 1), ('respawn_inventory', 4, 1),
    ('god', 1, 1), ('auto_run', 4, 1), ('crosshair', 4, 1), ('monsters_off', 4, 1),
    ('last_level', 2, 1), ('eog', 1, 1), ('coop', 4, 1), ('marker', 4, 1),
    ('ffire', 4, 1), ('camsprite', 2, 1), ('connecthead', 2, 1),
    ('connectpoint2', 32, 1), ('numplayersprites', 1, 1), ('frags', 512, 1),
    ('randomseed', 4, 1), ('global_random', 2, 1), ('parallaxyscale', 4, 1),
]


def uncompress(block):
    """One LZW block (without its 2-byte length) -> its bytes."""
    uncompleng, strtot = struct.unpack_from('<Hh', block, 0)
    if strtot == 0:
        return block[4:4 + uncompleng]
    data = block + b'\0\0\0\0'
    lzwbuf2 = list(range(256)) + [0] * (strtot + 2)
    lzwbuf3 = list(range(256)) + [0] * (strtot + 2)
    out = bytearray()
    currstr, bitcnt, numbits, oneupnumbits = 256, 32, 8, 256
    while True:
        at = bitcnt >> 3
        dat = (int.from_bytes(data[at:at + 4], 'little') >> (bitcnt & 7)) & (oneupnumbits - 1)
        bitcnt += numbits
        half = (oneupnumbits >> 1) - 1
        if (dat & half) > ((currstr - 1) & half):
            dat &= half
            bitcnt -= 1
        lzwbuf3[currstr] = dat
        tail = []
        while dat >= 256:
            tail.append(lzwbuf2[dat])
            dat = lzwbuf3[dat]
        out.append(dat)
        out.extend(reversed(tail))
        lzwbuf2[currstr - 1] = dat
        lzwbuf2[currstr] = dat
        currstr += 1
        if currstr > oneupnumbits:
            numbits += 1
            oneupnumbits <<= 1
        if currstr >= strtot:
            break
    return bytes(out[:uncompleng])


class Save:
    def __init__(self, data):
        self.data = data
        self.blocks = []        # raw blocks, each with its 2-byte length
        self.calls = {}         # name -> the call's bytes, delta-decoded
        self.call_blocks = {}   # name -> (first block index, block count)
        o = 0
        for name, dasizeof, count in CALLS:
            if count is None:
                count = struct.unpack('<h', self.calls['spriteqamount'])[0]
            first = len(self.blocks)
            self.calls[name], o = self._call(o, dasizeof, count)
            self.call_blocks[name] = (first, len(self.blocks) - first)
        if o != len(data):
            raise ValueError('%d bytes left after the last field' % (len(data) - o))

    def _block(self, o):
        if o + 2 > len(self.data):
            raise ValueError('file ends at byte %d' % o)
        leng = struct.unpack_from('<h', self.data, o)[0]
        raw = self.data[o:o + 2 + leng]
        if leng < 4 or len(raw) != 2 + leng:
            raise ValueError('bad block at byte %d' % o)
        self.blocks.append(raw)
        return uncompress(raw[2:]), o + 2 + leng

    def _call(self, o, dasizeof, count):
        # kdfread: each element after the first is a byte-wise delta from
        # the one before; a new block starts when the current one is used up.
        if dasizeof > LZWSIZE:
            count, dasizeof = count * dasizeof, 1
        block, o = self._block(o)
        out = bytearray(block[:dasizeof])
        k = dasizeof
        for _ in range(1, count):
            if k >= len(block):
                block, o = self._block(o)
                k = 0
            prev = out[-dasizeof:]
            out += bytes((prev[j] + block[k + j]) & 255 for j in range(dasizeof))
            k += dasizeof
        return bytes(out), o


def read(path):
    with open(path, 'rb') as f:
        return Save(f.read())


def state(save):
    c = save.calls
    ps0 = c['ps'][:PLAYER_SIZE]
    i = get(PLAYER_OFFS, ps0, 'i')
    spr = c['sprite'][i * SPRITE_SIZE:(i + 1) * SPRITE_SIZE]
    weapon = get(PLAYER_OFFS, ps0, 'curr_weapon')
    return 'SAVESTATE %d %d %d %d %d %d %d %d' % (
        get(SPRITE_OFFS, spr, 'extra'),
        get(PLAYER_OFFS, ps0, 'shield_amount'),
        weapon,
        get(PLAYER_OFFS, ps0, 'ammo_amount', weapon),
        struct.unpack('<i', c['volume'])[0],
        struct.unpack('<i', c['level'])[0],
        get(PLAYER_OFFS, ps0, 'posx'),
        get(PLAYER_OFFS, ps0, 'posy'))


def masked(name, data):
    """Bytes that may differ after a load and an immediate save."""
    if name in ('scriptptrs', 'hitptrs'):
        return b''      # offset bitmaps: DOS fills them, the port writes zeros
    if name == 'ps':
        data = bytearray(data)
        for p in range(MAXPLAYERS):
            o = p * PLAYER_SIZE + PLAYER_OFFS['palette'][0]
            data[o:o + 4] = b'\0\0\0\0'     # a DOS pointer, never restored
        data[PLAYER_OFFS['gm'][0]] = 0      # loadplayer sets ps[0].gm to MODE_GAME
        return bytes(data)
    return data


def compare(a, b):
    bad = 0
    for name, _, _ in CALLS:
        x, y = masked(name, a.calls[name]), masked(name, b.calls[name])
        if x != y:
            n = min(len(x), len(y))
            at = next((j for j in range(n) if x[j] != y[j]), n)
            print('%s differs at byte %d (%d vs %d bytes)' % (name, at, len(x), len(y)))
            bad = 1
    return bad


def fixture(kind, save, out):
    if kind == 'version':
        data = bytearray(save.data)
        if data[0:2] != b'\x08\x00' or data[4:6] != b'\x00\x00':
            raise ValueError('the version block is not stored uncompressed')
        data[6:10] = struct.pack('<i', 103)
    else:
        # The player block as an unpacked 64-bit build wrote it: 920 bytes a
        # player. A stored block (string count 0) needs no compressor.
        first, count = save.call_blocks['ps']
        ps = save.calls['ps']
        old = b''.join(ps[p * PLAYER_SIZE:(p + 1) * PLAYER_SIZE] + b'\0' * 9
                       for p in range(MAXPLAYERS))
        stored = struct.pack('<hHh', 4 + len(old), len(old), 0) + old
        data = (b''.join(save.blocks[:first]) + stored
                + b''.join(save.blocks[first + count:]))
    with open(out, 'wb') as f:
        f.write(data)


def main(argv):
    cmd = argv[1] if len(argv) > 1 else ''
    if cmd == 'check' and len(argv) == 3:
        save = read(argv[2])
        version = struct.unpack('<i', save.calls['version'])[0]
        if version != BYTEVERSION:
            print('version %d, not %d' % (version, BYTEVERSION))
            return 1
        print('ok')
    elif cmd == 'state' and len(argv) == 3:
        print(state(read(argv[2])))
    elif cmd == 'compare' and len(argv) == 4:
        return compare(read(argv[2]), read(argv[3]))
    elif cmd == 'fixture' and len(argv) == 5 and argv[2] in ('oldlayout', 'version'):
        fixture(argv[2], read(argv[3]), argv[4])
    else:
        print(__doc__)
        return 2
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv))
    except ValueError as e:
        print('error: %s' % e)
        sys.exit(1)
