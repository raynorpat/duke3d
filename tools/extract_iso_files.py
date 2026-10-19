#!/usr/bin/env python3
"""Copy files out of an ISO 9660 CD image without changing it.

  extract_iso_files.py <image> <dest dir> <path on disc>...

The image is a plain .iso (2048-byte sectors) or a raw MODE1/2352 .bin.
Paths are like ATOMINST/DUKE3D.GRP and match case-insensitively; the ";1"
version suffix is ignored. Files are written to <dest dir> under their own
name.
"""
import os
import struct
import sys


class Image:
    def __init__(self, path):
        self.f = open(path, 'rb')
        self.raw = False
        self.f.seek(16 * 2352 + 16 + 1)
        if self.f.read(5) == b'CD001':
            self.raw = True
        elif self.sector(16)[1:6] != b'CD001':
            raise SystemExit('%s: no ISO 9660 volume found' % path)

    def sector(self, lba):
        if self.raw:
            self.f.seek(lba * 2352 + 16)
        else:
            self.f.seek(lba * 2048)
        return self.f.read(2048)

    def extent(self, lba, size):
        out = bytearray()
        while len(out) < size:
            out += self.sector(lba)
            lba += 1
        return bytes(out[:size])

    def entries(self, lba, size):
        data = self.extent(lba, size)
        o = 0
        while o < len(data):
            length = data[o]
            if length == 0:
                o = (o // 2048 + 1) * 2048
                continue
            rec = data[o:o + length]
            name = rec[33:33 + rec[32]]
            if name not in (b'\0', b'\1'):
                yield (name.decode('ascii').split(';')[0].upper(),
                       struct.unpack_from('<I', rec, 2)[0],
                       struct.unpack_from('<I', rec, 10)[0],
                       bool(rec[25] & 2))
            o += length

    def find(self, path):
        pvd = self.sector(16)
        lba = struct.unpack_from('<I', pvd, 156 + 2)[0]
        size = struct.unpack_from('<I', pvd, 156 + 10)[0]
        parts = path.upper().split('/')
        for depth, part in enumerate(parts):
            for name, elba, esize, is_dir in self.entries(lba, size):
                if name == part and is_dir == (depth < len(parts) - 1):
                    lba, size = elba, esize
                    break
            else:
                raise SystemExit('%s: not on the disc' % path)
        return lba, size


def main(argv):
    if len(argv) < 4:
        print(__doc__)
        return 2
    image = Image(argv[1])
    os.makedirs(argv[2], exist_ok=True)
    for path in argv[3:]:
        lba, size = image.find(path)
        dest = os.path.join(argv[2], path.split('/')[-1].upper())
        with open(dest, 'wb') as out:
            out.write(image.extent(lba, size))
        print('%s -> %s (%d bytes)' % (path, dest, size))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
