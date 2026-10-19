"""Flatten a DOS/4G LE executable: load objects at their link bases, apply internal
fixups, write one raw image starting at the lowest base. Prints a summary."""
import struct, sys
src, leoff, out = sys.argv[1], int(sys.argv[2], 16), sys.argv[3]
stub = int(sys.argv[4], 16) if len(sys.argv) > 4 else 0
d = open(src, 'rb').read()
H = lambda o, f: struct.unpack_from(f, d, leoff + o)[0]
npages, pagesize, lastpage = H(0x14,'<I'), H(0x28,'<I'), H(0x2c,'<I')
objtab, nobj, pmap = H(0x40,'<I'), H(0x44,'<I'), H(0x48,'<I')
fpt, frt, datapages = H(0x68,'<I'), H(0x6c,'<I'), H(0x80,'<I')
objs = [struct.unpack_from('<6I', d, leoff + objtab + i*24) for i in range(nobj)]
lo = min(o[1] for o in objs); hi = max(o[1] + o[0] for o in objs)
img = bytearray(hi - lo)
pagebase = {}  # logical page index (1-based) -> image address
for vs, base, fl, pmi, pmn, _ in objs:
    for j in range(pmn):
        e = d[leoff + pmap + (pmi - 1 + j)*4: leoff + pmap + (pmi + j)*4]
        num = (e[0] << 16) | (e[1] << 8) | e[2]
        size = lastpage if num == npages else pagesize
        a = base + j*pagesize
        pagebase[pmi + j] = a
        if e[3] == 0 and num:
            chunk = d[stub + datapages + (num-1)*pagesize: stub + datapages + (num-1)*pagesize + size]
            img[a-lo:a-lo+len(chunk)] = chunk
nfix = {}
for p in range(1, npages+1):
    s = struct.unpack_from('<I', d, leoff + fpt + (p-1)*4)[0]
    e = struct.unpack_from('<I', d, leoff + fpt + p*4)[0]
    i = leoff + frt + s; end = leoff + frt + e
    while i < end:
        st, fl = d[i], d[i+1]; i += 2
        if st & 0x20:
            cnt = d[i]; i += 1; srcs = None
        else:
            srcs = [struct.unpack_from('<h', d, i)[0]]; i += 2
        assert fl & 3 == 0, f'non-internal fixup flags={fl:#x} page={p}'
        if fl & 0x40: tobj = struct.unpack_from('<H', d, i)[0]; i += 2
        else: tobj = d[i]; i += 1
        t = st & 0xf
        toff = 0
        if t != 2:
            if fl & 0x10: toff = struct.unpack_from('<I', d, i)[0]; i += 4
            else: toff = struct.unpack_from('<H', d, i)[0]; i += 2
        if srcs is None:
            srcs = [struct.unpack_from('<h', d, i + 2*k)[0] for k in range(cnt)]; i += 2*cnt
        tgt = objs[tobj-1][1] + toff
        for so in srcs:
            a = pagebase[p] + so - lo
            if t == 7: struct.pack_into('<I', img, a, tgt)
            elif t == 8: struct.pack_into('<i', img, a, tgt - (a + lo + 4))
            elif t == 2: struct.pack_into('<H', img, a, tobj)   # selector: keep object number
            elif t == 6: struct.pack_into('<IH', img, a, tgt, tobj)
            elif t == 5: struct.pack_into('<H', img, a, tgt & 0xffff)
            else: raise SystemExit(f'unhandled fixup type {st:#x}')
            nfix[t] = nfix.get(t, 0) + 1
open(out, 'wb').write(img)
eip = objs[H(0x18,'<I')-1][1] + H(0x1c,'<I')
print(f'base={lo:#x} size={len(img):#x} entry={eip:#x} fixups={nfix}')
for k, (vs, base, fl, *_ ) in enumerate(objs, 1): print(f'obj{k} {base:#x}-{base+vs:#x} flags={fl:#x}')
