"""LE page mapping and save-hook placement without copyrighted executable data."""
import importlib.util
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location(
    'capture_le', Path(__file__).resolve().parents[1] / 'tools/v13d/le.py')
le = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(le)


class CaptureTools(unittest.TestCase):
    def image(self):
        image = bytearray(0x4000)
        # Two virtual pages, stored in reverse order in the executable.
        for offset, value in ((0x40, 0x100), (0x44, 1), (0x48, 0x120), (0x80, 0x1000)):
            struct.pack_into('<I', image, offset, value)
        struct.pack_into('<6I', image, 0x100, 0x2000, 0x10000, 0, 1, 2, 0)
        image[0x120:0x128] = bytes.fromhex('0000020000000100')
        return le.LEImage(image, dict(le=0, stub=0))

    def test_page_mapping_and_patch(self):
        image = self.image()
        image.wr(0x10025, b'first')
        image.wr(0x11025, b'second')
        self.assertEqual(image.exe[0x2025:0x202a], b'first')
        self.assertEqual(image.exe[0x1025:0x102b], b'second')
        self.assertEqual(image.rd(0x10025, 5), b'first')
        with self.assertRaises(SystemExit):
            image.foff(0x12000)

    def test_save_hook_follows_capture_payload(self):
        image = self.image()
        edition = dict(cave=0x10000, cavesize=0x900)
        for size, hook in ((213, 0x10100), (300, 0x10200)):
            image.wr(edition['cave'], bytes(edition['cavesize']))
            image.wr(edition['cave'] + size - len(le.CAPTURE_FILE), le.CAPTURE_FILE)
            self.assertEqual(image.save_hook(edition), hook)

    def test_save_hook_rejects_missing_or_duplicate_marker(self):
        image = self.image()
        edition = dict(cave=0x10000, cavesize=0x900)
        with self.assertRaises(AssertionError):
            image.save_hook(edition)
        image.wr(0x10020, le.CAPTURE_FILE)
        image.wr(0x10120, le.CAPTURE_FILE)
        with self.assertRaises(AssertionError):
            image.save_hook(edition)

    def test_save_hook_rejects_cave_overflow(self):
        image = self.image()
        edition = dict(cave=0x10000, cavesize=0x120)
        image.wr(0x10100, le.CAPTURE_FILE)
        with self.assertRaises(AssertionError):
            image.save_hook(edition)

    def test_save_name_rejected_before_patching(self):
        tool = Path(__file__).resolve().parents[1] / 'tools/v13d/mksave.py'
        with tempfile.TemporaryDirectory() as work:
            out = Path(work) / 'out.exe'
            for name in ('x'*19, 'caf\u00e9'):
                result = subprocess.run(
                    [sys.executable, '-I', str(tool), 'sw11', 'missing.exe',
                     str(out), '1', '40', '4', name], capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('save name must be at most 18 ASCII characters', result.stderr)
                self.assertFalse(out.exists())


if __name__ == '__main__':
    unittest.main()
