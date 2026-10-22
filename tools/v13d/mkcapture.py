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
import importlib.util, os, re, struct, sys
# Load by path so the tools also work with Python's isolated (-I) mode.
spec = importlib.util.spec_from_file_location('capture_le', os.path.join(os.path.dirname(os.path.abspath(__file__)), 'le.py'))
le = importlib.util.module_from_spec(spec); spec.loader.exec_module(le)
ED = le.ED
ed = ED[sys.argv[1]]
FLAT = __import__('os').environ.get('V13D_FLAT') or __import__('os').path.expanduser('~/Projects/ref/ghidra/' + ed.get('flat', f'{sys.argv[1]}13.bin'))
LOGN = ed.get('logn', 256); exe = bytearray(open(sys.argv[2], 'rb').read()); demo = int(sys.argv[4])
image = le.LEImage(exe, ed); rd, wr = image.rd, image.wr
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
fname = cave + len(c); c += le.CAPTURE_FILE
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
if sys.argv[1] == 'sw099':
    # Playback returns on MODE_EOL before reaching the normal record-end branch.
    assert rd(0x2a8ab, 13) == bytes.fromhex('240825ff0000000f85fd000000')
    wr(0x2a8b4, struct.pack('<i', cave - 0x2a8b8))
if 'demogod' in sys.argv[5:]:
    assert sys.argv[1] == 'sw099', 'demogod is a beta reference option'
    # checkcommandline: xor ah,ah; ...; mov byte [god],ah. No later reset.
    assert rd(0x293aa, 2) == b'\x30\xe4', 'unexpected beta god initializer'
    wr(0x293aa, b'\xb4\x01')  # mov ah,1
    print('demogod: initialize beta god mode at startup')
wr(ed['which_demo'], bytes([demo]))
open(sys.argv[3], 'wb').write(exe)
print(f'jnz at {jnz:#x} (was -> {end:#x}), cave at {cave:#x}, {len(c)} bytes, start demo {demo}')
