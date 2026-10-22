"""Make a scratch copy of a DUKE3D.EXE (1.3D full or shareware, or 1.1, 1.0 or 0.99
shareware) that, when a demo ends, writes the game state to STATE.BIN and then
hangs. Optionally starts the title demo loop at demo N.

usage: mkcapture.py <full|sw|sw11|sw10|sw099> <in.exe> <out.exe> <demo number> [krandlog] [demogod]

STATE.BIN = reccnt (4) + randomseed (4) + sprite[4096] (44 each) + ps[0] (0x552, the
1.3D player_struct layout, see ledger F-007; 0x34e for sw11 and sw10, see
docs/v10-v11/reference.md; 0x33f for sw099, docs/v099/reference.md).

krandlog: every direct call to krand goes through a logger first, which records
the caller while reccnt is 0 (the demo's last tic, when the demo is cut to end
there). STATE.BIN then also holds the runtime and link address of the logger
(4 + 4), the call count (4) and up to 256 return addresses (4 each, runtime;
subtract the runtime-minus-link difference to get EXE addresses; sw11 has room
for 41). The code object is read-only, so the log lives in the slack after the
data object."""
import re, struct, sys
ED = {
    'full': dict(krand=0x6c241, randomseed=0x251998, sprite=0x1cdbc0, reccnt=0x14de0a, which_demo=0xc76d6, cave=0xaa620, log=0x2d5400, ps=0x11bd30),
    'sw':   dict(krand=0x6bf61, randomseed=0x251ae8, sprite=0x1cdd10, reccnt=0x14df5a, which_demo=0xc782e, cave=0xaa330, log=0x2d5550, ps=0x11be80),
    # 1.1 and 1.0 shareware (docs/v10-v11/reference.md): other LE offsets, 8 x 0x34e
    # ps[], a smaller code cave, and only 0xb0 bytes of log slack in 1.1
    'sw11': dict(krand=0x67a71, randomseed=0x22ae04, sprite=0x1a7030, reccnt=0x130562, which_demo=0xc6c8d, cave=0xa4700, log=0x2abf50, ps=0x1162e0,
                 le=0x290fc, stub=0x26654, cavesize=0x900, pssize=0x34e, logn=41, flat='sw11.bin'),
    'sw10': dict(krand=0x661e7, randomseed=0x22aaf0, sprite=0x1a6d1c, reccnt=0x130ebb, which_demo=0xc6783, cave=0xa2aa0, log=0x2abaf0, ps=0x112ab0,
                 le=0x290fc, stub=0x26654, cavesize=0x560, pssize=0x34e, flat='sw10.bin'),
    'sw099': dict(krand=0x6151e, randomseed=0x210c30, sprite=0x18ce5c, reccnt=0xf0cd8, which_demo=0xa5866, cave=0x99b20, log=0x2471a0, ps=0xf0f38,
                  le=0x2a50, stub=0, cavesize=0x4e0, pssize=0x33f, flat='beta099.bin', demostack=8, codeend=0x9a000),
}
ed = ED[sys.argv[1]]
LE, STUB = ed.get('le', 0x37c3c), ed.get('stub', 0x352a4)
FLAT = __import__('os').environ.get('V13D_FLAT') or __import__('os').path.expanduser('~/Projects/ref/ghidra/' + ed.get('flat', f'{sys.argv[1]}13.bin'))
LOGN = ed.get('logn', 256); exe = bytearray(open(sys.argv[2], 'rb').read()); demo = int(sys.argv[4])
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
# the demo-end branch in playback: cmp [reccnt],0 / jg / cmp byte [esp+4],0 / jnz END (esp+8 in 0.99)
code = open(FLAT, 'rb').read()[:0x30000]  # flattened image (le_flatten.py), fixups applied
pat = re.compile(rb'\x83\x3d' + re.escape(struct.pack('<I', ed['reccnt'])) + rb'\x00\x7f.\x80\x7c\x24' + bytes([ed.get('demostack', 4)]) + rb'\x00\x0f\x85(....)', re.S)
m = list(pat.finditer(code)); assert len(m) == 1, len(m)
jnz = 0x10000 + m[0].start() + 14           # address of the 0f 85
end = jnz + 6 + struct.unpack('<i', m[0].group(1))[0]
cave = ed['cave']; assert not any(rd(cave, ed.get('cavesize', 0x600))), 'cave not empty'
c = bytearray(); nxt = cave + 5
c += b'\xe8\x00\x00\x00\x00'                 # call next
c += b'\x5d'                                 # next: pop ebp
c += b'\x8b\xb5' + struct.pack('<i', ed['krand'] + 2 - nxt)  # mov esi,[ebp+...] = &randomseed (runtime)
c += b'\x81\xee' + struct.pack('<I', ed['randomseed'])      # sub esi, link addr -> data delta
fname_fix = len(c) + 6  # offset of the lea disp32
c += b'\xb4\x3c\x31\xc9\x8d\x95\x00\x00\x00\x00\xcd\x21'    # mov ah,3c; xor ecx,ecx; lea edx,[ebp+fname]; int 21
c += b'\x89\xc7'                             # mov edi,eax (handle)
def write(addr, n):
    global c
    c += b'\x66\x89\xfb' + b'\xb9' + struct.pack('<I', n) + b'\x8d\x96' + struct.pack('<I', addr) + b'\xb4\x40\xcd\x21'
write(ed['reccnt'], 4); write(ed['randomseed'], 4)
for k in range(5): write(ed['sprite'] + k*32768, 32768)
write(ed['sprite'] + 5*32768, 44*4096 - 5*32768)
write(ed['ps'], ed.get('pssize', 0x552))
krandlog = 'krandlog' in sys.argv[5:]
if krandlog:
    write(ed['log'], 12 + LOGN*4)
c += b'\x66\x89\xfb\xb4\x3e\xcd\x21'         # close
c += b'\xeb\xfe'                             # hang
fname = cave + len(c); c += b'STATE.BIN\x00'
struct.pack_into('<i', c, fname_fix, fname - nxt)
if krandlog:
    hook = cave + len(c); hn = hook + 9      # address of the pop after its call
    log = ed['log']                          # log: hn runtime, hn link, count, addrs[256]
    h = bytearray(b'\x53\x50\x51\x52\xe8\x00\x00\x00\x00\x5b')
    h += b'\x8b\x93' + struct.pack('<i', ed['krand'] + 2 - hn)        # mov edx,[ebx+...] (&randomseed)
    h += b'\x81\xea' + struct.pack('<I', ed['randomseed'])          # sub edx, link -> data delta
    h += b'\x83\xba' + struct.pack('<I', ed['reccnt']) + b'\x00'     # cmp dword [edx+reccnt],0
    h += b'\x75\x00'; jne = len(h)
    h += b'\x89\x9a' + struct.pack('<I', log)                       # log[0] = runtime hn ([edx+log])
    h += b'\xc7\x82' + struct.pack('<I', log + 4) + struct.pack('<I', hn)  # log[1] = link hn
    h += b'\x8b\x8a' + struct.pack('<I', log + 8)                   # ecx = count
    h += b'\x81\xf9' + struct.pack('<I', LOGN) + b'\x73\x00'; jae = len(h)  # cmp ecx,LOGN / jae skip
    h += b'\x8b\x44\x24\x10'                                         # eax = caller
    h += b'\x89\x84\x8a' + struct.pack('<I', log + 12)             # log[3+ecx] = eax ([edx+ecx*4])
    h += b'\x41\x89\x8a' + struct.pack('<I', log + 8)              # count++
    skip = len(h); h[jne-1] = skip - jne; h[jae-1] = skip - jae
    h += b'\x5a\x59\x58\x5b\xe9'
    h += struct.pack('<i', ed['krand'] - (hook + len(h) + 4))   # jmp krand
    c += bytes(h)
    whole = open(FLAT, 'rb').read()
    sites = [k + 0x10000 for k in range(0, ed.get('codeend', 0xb0000) - 0x10000 - 5) if whole[k] == 0xe8
             and k + 0x10000 + 5 + struct.unpack_from('<i', whole, k + 1)[0] == ed['krand']]
    for k in sites: wr(k + 1, struct.pack('<i', hook - (k + 5)))
    print(f'krandlog: {len(sites)} krand calls go through {hook:#x}')
wr(cave, bytes(c))
if len(sys.argv) > 5 and sys.argv[5] == 'krandlog':
    wr(jnz + 2, struct.pack('<i', cave - (jnz + 6)))
elif len(sys.argv) > 5 and sys.argv[5] == 'callsite':
    k = int(sys.argv[6], 16); assert code[k-0x10000] == 0xe8, 'not a call'
    wr(k + 1, struct.pack('<i', cave - (k + 5))); print(f'diagnostic: call at {k:#x} -> cave')
elif len(sys.argv) > 5 and sys.argv[5] == 'firsttic':
    # diagnostic: divert playback's first domovethings call (e8 rel32 right after the tic loop) to the cave
    m2 = re.search(rb'\xe8(....)\xe9', code[jnz-0x10000:jnz-0x10000+0x200], re.S)
    calls = [k for k in range(jnz, jnz+0x200) if code[k-0x10000] == 0xe8 and k+5+struct.unpack('<i', code[k-0x10000+1:k-0x10000+5])[0] == int(sys.argv[6], 16)]
    assert calls, 'domovethings call not found'
    k = calls[1] if len(calls) > 1 else calls[0]; wr(k + 1, struct.pack('<i', cave - (k + 5))); print(f'diagnostic: call at {k:#x} -> cave')
else:
    wr(jnz + 2, struct.pack('<i', cave - (jnz + 6)))
if 'demogod' in sys.argv[5:]:
    assert sys.argv[1] == 'sw099', 'demogod is a beta reference option'
    # checkcommandline: xor ah,ah; ...; mov byte [god],ah. No later reset.
    assert rd(0x293aa, 2) == b'\x30\xe4', 'unexpected beta god initializer'
    wr(0x293aa, b'\xb4\x01')  # mov ah,1
    print('demogod: initialize beta god mode at startup')
wr(ed['which_demo'], bytes([demo]))
open(sys.argv[3], 'wb').write(exe)
print(f'jnz at {jnz:#x} (was -> {end:#x}), cave at {cave:#x}, {len(c)} bytes, start demo {demo}')
