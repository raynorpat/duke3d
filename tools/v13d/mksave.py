"""Make a scratch copy of a 1.1 or 1.0 shareware DUKE3D.EXE that plays demo N and,
right after tic T, saves the game into slot S with the game's own saveplayer
(GAME<S>.SAV), then writes STATE.BIN and hangs as mkcapture.py does. With a
generated demo (gendemo.py, optionally on a startnear.py GRP) this makes DOS
reference saves without anyone at the keyboard.

usage: mksave.py <sw11|sw10> <in.exe> <out.exe> <demo number> <tic> <slot>
       mksave.py values <sw11|sw10> <STATE.BIN> <name> <level, like E1L1>

The first form runs mkcapture.py, then diverts playback's call to domovethings to
a hook in the code cave after mkcapture's patch. The hook counts tics in the data
object's slack (mkcapture's log address). At tic T it calls saveplayer(S) and jumps
to mkcapture's STATE.BIN dump. If the demo ends first, mkcapture's own dump runs
and no save is written.

The second form prints the save's expected values (tests/data/v13d_save_refs.cmake)
from that STATE.BIN, as V13D_SAVE_<edition>_<name>_* for refsaves/<name>.SAV:
health is the player sprite's extra, armor ps[0].shield_amount,
ammo ps[0].ammo_amount[curr_weapon], X/Y the player sprite's."""
import os, re, struct, subprocess, sys
ED = {  # docs/v10-v11/reference.md, "Address map"
    'sw11': dict(krand=0x67a71, randomseed=0x22ae04, log=0x2abf50, cave=0xa4700, domovethings=0x20ff0, saveplayer=0x38f60, flat='sw11.bin'),
    'sw10': dict(krand=0x661e7, randomseed=0x22aaf0, log=0x2abaf0, cave=0xa2aa0, domovethings=0x20ae0, saveplayer=0x388d0, flat='sw10.bin'),
}
LE, STUB, PS = 0x290fc, 0x26654, 0x34e   # both releases

if sys.argv[1] == 'values':
    d = open(sys.argv[3], 'rb').read(); name, level = sys.argv[4], sys.argv[5]
    assert len(d) == 8 + 44*4096 + PS, f'{len(d)} bytes: not a {sys.argv[2]} STATE.BIN'
    ps = d[8 + 44*4096:]
    i = struct.unpack_from('<h', ps, 0x2ca)[0]                     # ps.i, the player sprite
    x, y = struct.unpack_from('<ii', d, 8 + 44*i)
    picnum, extra = struct.unpack_from('<h', d, 8 + 44*i + 0xe)[0], struct.unpack_from('<h', d, 8 + 44*i + 0x2a)[0]
    assert picnum == 1405, f'ps.i {i} is picnum {picnum}, not APLAYER'
    cur = struct.unpack_from('<h', ps, 0x29e)[0]
    ammo = struct.unpack_from('<h', ps, 0x288 + 2*cur)[0]
    armor = struct.unpack_from('<h', ps, 0x2f6)[0]
    for k, v in (('EDITION', sys.argv[2]), ('FILE', name + '.SAV'), ('HEALTH', extra), ('ARMOR', armor),
                 ('CURWEAPON', cur), ('AMMO', ammo), ('LEVEL', level), ('X', x), ('Y', y)):
        print(f'set(V13D_SAVE_{sys.argv[2]}_{name}_{k} {v})')
    sys.exit()

edition, src, out, demo, tic, slot = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], int(sys.argv[5]), int(sys.argv[6])
ed = ED[edition]
subprocess.run([sys.executable, '-I', os.path.join(os.path.dirname(os.path.abspath(__file__)), 'mkcapture.py'),
                edition, src, out, demo], check=True)
exe = bytearray(open(out, 'rb').read())
H = lambda o, f: struct.unpack_from(f, exe, LE + o)[0]
objtab, pmap, datapages = H(0x40, '<I'), H(0x48, '<I'), H(0x80, '<I')
objs = [struct.unpack_from('<6I', exe, LE + objtab + i*24) for i in range(H(0x44, '<I'))]
def foff(addr):
    for vs, base, fl, pmi, pmn, _ in objs:
        if base <= addr < base + pmn*0x1000:
            j = (addr - base) // 0x1000
            e = exe[LE + pmap + (pmi - 1 + j)*4: LE + pmap + (pmi + j)*4]
            num = (e[0] << 16) | (e[1] << 8) | e[2]
            return STUB + datapages + (num - 1)*0x1000 + (addr - base) % 0x1000
    raise SystemExit(f'{addr:#x} not in a file-backed page')
def rd(addr, n): o = foff(addr); return bytes(exe[o:o+n])
def wr(addr, b): o = foff(addr); exe[o:o+len(b)] = b
rel = lambda target, after: struct.pack('<i', target - after)
# playback's domovethings call: the only call to it between the demo-end jnz and the
# loop's jmp back (mkcapture's own search pattern finds the jnz)
code = open(os.environ.get('V13D_FLAT') or os.path.expanduser('~/Projects/ref/ghidra/' + ed['flat']), 'rb').read()[:0x30000]
pat = re.compile(rb'\x83\x3d....\x00\x7f.\x80\x7c\x24\x04\x00\x0f\x85', re.S)
jnz = [0x10000 + m.start() + 14 for m in pat.finditer(code)]; assert len(jnz) == 1, jnz
sites = [k for k in range(jnz[0], jnz[0] + 0x200) if code[k-0x10000] == 0xe8
         and k + 5 + struct.unpack_from('<i', code, k - 0x10000 + 1)[0] == ed['domovethings']]
assert len(sites) == 1, sites
hook = ed['cave'] + 0x100                    # after mkcapture's patch (213 bytes)
h = bytearray()
h += b'\xe8' + rel(ed['domovethings'], hook + 5)                 # call domovethings
h += b'\x60\xe8\x00\x00\x00\x00\x5d'; nxt = hook + len(h) - 1    # pushad; call next; next: pop ebp
h += b'\x8b\xb5' + rel(ed['krand'] + 2, nxt)                     # mov esi,[&randomseed] (runtime)
h += b'\x81\xee' + struct.pack('<I', ed['randomseed'])           # sub esi, link addr -> data delta
h += b'\xff\x86' + struct.pack('<I', ed['log'])                  # inc dword [esi+log] (tic count)
h += b'\x81\xbe' + struct.pack('<II', ed['log'], tic)            # cmp dword [esi+log], tic
h += b'\x75\x00'; jne = len(h)
h += b'\xb8' + struct.pack('<I', slot)                           # mov eax, slot
h += b'\xe8' + rel(ed['saveplayer'], hook + len(h) + 5)          # call saveplayer
h += b'\xe9' + rel(ed['cave'], hook + len(h) + 5)                # jmp to mkcapture's dump (hangs)
h[jne-1] = len(h) - jne
h += b'\x61\xc3'                                                 # popad; ret
assert not any(rd(hook, len(h))), 'cave after the capture patch not empty'
wr(hook, bytes(h))
wr(sites[0] + 1, rel(hook, sites[0] + 5))
open(out, 'wb').write(exe)
print(f'domovethings call at {sites[0]:#x} -> hook {hook:#x} ({len(h)} bytes): save to slot {slot} after tic {tic}')
