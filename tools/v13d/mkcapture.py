"""Make a scratch copy of a 1.3D DUKE3D.EXE that, when a demo ends, writes the
game state to STATE.BIN and then hangs. Optionally starts the title demo loop at
demo N.

usage: mkcapture.py <full|sw> <in.exe> <out.exe> <demo number>

STATE.BIN = reccnt (4) + randomseed (4) + sprite[4096] (44 each)."""
import re, struct, sys
ED = {
    'full': dict(krand=0x6c241, randomseed=0x251998, sprite=0x1cdbc0, reccnt=0x14de0a, which_demo=0xc76d6, cave=0xaa620),
    'sw':   dict(krand=0x6bf61, randomseed=0x251ae8, sprite=0x1cdd10, reccnt=0x14df5a, which_demo=0xc782e, cave=0xaa330),
}
LE, STUB = 0x37c3c, 0x352a4
ed = ED[sys.argv[1]]; exe = bytearray(open(sys.argv[2], 'rb').read()); demo = int(sys.argv[4])
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
# the demo-end branch in playback: cmp [reccnt],0 / jg / cmp byte [esp+4],0 / jnz END
code = open(__import__('os').environ.get('V13D_FLAT') or __import__('os').path.expanduser(f'~/Projects/ref/ghidra/{sys.argv[1]}13.bin'), 'rb').read()[:0x30000]  # flattened image (le_flatten.py), fixups applied
pat = re.compile(rb'\x83\x3d' + re.escape(struct.pack('<I', ed['reccnt'])) + rb'\x00\x7f.\x80\x7c\x24\x04\x00\x0f\x85(....)', re.S)
m = list(pat.finditer(code)); assert len(m) == 1, len(m)
jnz = 0x10000 + m[0].start() + 14           # address of the 0f 85
end = jnz + 6 + struct.unpack('<i', m[0].group(1))[0]
cave = ed['cave']; assert not any(rd(cave, 0x200)), 'cave not empty'
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
c += b'\x66\x89\xfb\xb4\x3e\xcd\x21'         # close
c += b'\xeb\xfe'                             # hang
fname = cave + len(c); c += b'STATE.BIN\x00'
struct.pack_into('<i', c, fname_fix, fname - nxt)
wr(cave, bytes(c))
if len(sys.argv) > 5 and sys.argv[5] == 'callsite':
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
wr(ed['which_demo'], bytes([demo]))
open(sys.argv[3], 'wb').write(exe)
print(f'jnz at {jnz:#x} (was -> {end:#x}), cave at {cave:#x}, {len(c)} bytes, start demo {demo}')
