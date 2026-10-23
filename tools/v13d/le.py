"""Edition addresses and LE page access for the DOS capture/save patchers."""
import struct

CAPTURE_FILE = b'STATE.BIN\x00'
ED = {
    'full': dict(krand=0x6c241, randomseed=0x251998, sprite=0x1cdbc0, reccnt=0x14de0a, which_demo=0xc76d6, cave=0xaa620, log=0x2d5400, ps=0x11bd30),
    'sw':   dict(krand=0x6bf61, randomseed=0x251ae8, sprite=0x1cdd10, reccnt=0x14df5a, which_demo=0xc782e, cave=0xaa330, log=0x2d5550, ps=0x11be80),
    # 1.1 and 1.0 shareware (docs/v10-v11/reference.md): other LE offsets, 8 x 0x34e
    # ps[], a smaller code cave, and only 0xb0 bytes of log slack in 1.1
    'sw11': dict(krand=0x67a71, randomseed=0x22ae04, sprite=0x1a7030, reccnt=0x130562, which_demo=0xc6c8d, cave=0xa4700, log=0x2abf50, ps=0x1162e0, domovethings=0x20ff0, saveplayer=0x38f60, savename=0x130420,
                 le=0x290fc, stub=0x26654, cavesize=0x900, pssize=0x34e, logn=41, flat='sw11.bin'),
    'sw10': dict(krand=0x661e7, randomseed=0x22aaf0, sprite=0x1a6d1c, reccnt=0x130ebb, which_demo=0xc6783, cave=0xa2aa0, log=0x2abaf0, ps=0x112ab0, domovethings=0x20ae0, saveplayer=0x388d0, savename=0x130d7a,
                 le=0x290fc, stub=0x26654, cavesize=0x560, pssize=0x34e, flat='sw10.bin'),
    'sw099': dict(krand=0x6151e, randomseed=0x210c30, sprite=0x18ce5c, reccnt=0xf0cd8, which_demo=0xa5866, cave=0x99b20, log=0x2471a0, ps=0xf0f38, domovethings=0x2bab0, saveplayer=0x35560, savename=0xf0bad,
                  player=0x2bc, weapon=0x290, ammo=0x27c, armor=0x318,
                  le=0x2a50, stub=0, cavesize=0x4e0, pssize=0x33f, flat='beta099.bin', demostack=8, codeend=0x9a000),
}


class LEImage:
    def __init__(self, exe, ed):
        self.exe = exe
        self.le, self.stub = ed.get('le', 0x37c3c), ed.get('stub', 0x352a4)
        header = lambda o: struct.unpack_from('<I', exe, self.le + o)[0]
        objtab, self.pmap, self.datapages = header(0x40), header(0x48), header(0x80)
        self.objs = [struct.unpack_from('<6I', exe, self.le + objtab + i*24)
                     for i in range(header(0x44))]

    def foff(self, addr):
        for vs, base, fl, pmi, pmn, _ in self.objs:
            if base <= addr < base + pmn*0x1000:
                j = (addr - base) // 0x1000
                e = self.exe[self.le + self.pmap + (pmi - 1 + j)*4:
                             self.le + self.pmap + (pmi + j)*4]
                num = (e[0] << 16) | (e[1] << 8) | e[2]
                return self.stub + self.datapages + (num - 1)*0x1000 + (addr - base) % 0x1000
        raise SystemExit(f'{addr:#x} not in a file-backed page')

    def rd(self, addr, n):
        o = self.foff(addr)
        return bytes(self.exe[o:o+n])

    def wr(self, addr, b):
        o = self.foff(addr)
        self.exe[o:o+len(b)] = b

    def save_hook(self, ed):
        # Without krandlog, the capture payload ends with its output filename.
        cave = self.rd(ed['cave'], ed.get('cavesize', 0x600))
        assert cave.count(CAPTURE_FILE) == 1, 'capture filename missing or ambiguous'
        used = cave.index(CAPTURE_FILE) + len(CAPTURE_FILE)
        hook = (used + 0xff) & ~0xff
        assert hook < len(cave), 'save hook outside code cave'
        return ed['cave'] + hook
