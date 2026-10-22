#!/usr/bin/env python3
"""Mutate copies of DOS saves and check refusal before loading game state."""
import argparse
import importlib.util
import os
from pathlib import Path
import struct
import subprocess
import sys

sys.dont_write_bytecode = True

spec = importlib.util.spec_from_file_location('decoder', Path(__file__).with_name('dos_save_decode.py'))
decoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decoder)


def parse(data, version):
    old = version in ('0.99', '1.0')
    early = version in ('0.99', '1.0', '1.1')
    atomic = version in ('1.4', '1.5')
    players, scriptlen = (8, 10240) if early else (16, 20460 if atomic else 20000)
    psize = 0x33f if version == '0.99' else (0x34e if early else (911 if atomic else 0x552))
    hits, tiles = (4096, 6144) if atomic else (2048, 4096)
    if not atomic:
        spec = importlib.util.spec_from_file_location('old_decoder', Path(__file__).with_name('v13d_save_decode.py'))
        old_decoder = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(old_decoder)
        decoder.uncompress = old_decoder.uncompress
    s = decoder.Save.__new__(decoder.Save)
    s.data, s.blocks, s.calls, s.call_blocks = data, [], {}, {}
    offset = 0

    def call(name, size, count):
        nonlocal offset
        first = len(s.blocks)
        s.calls[name], offset = s._call(offset, size, count)
        s.call_blocks[name] = first, len(s.blocks) - first

    def calls(items):
        for size, count in items:
            call(str(len(s.calls)), size, count)

    if not old:
        call('version', 4, 1)
    if atomic:
        call('multimode', 4, 1)
    calls([(19, 1), (4, 1), (4, 1), (4, 1), (160, 100)])
    call('numwalls', 2, 1)
    call('walls', 32, struct.unpack('<h', s.calls['numwalls'])[0] if old else 8192)
    call('numsectors', 2, 1)
    call('sectors', 40, struct.unpack('<h', s.calls['numsectors'])[0] if old else 1024)
    call('sprites', 44, 4096)
    calls([(2, 1025), (2, 4096), (2, 4096), (2, 1025), (2, 4096), (2, 4096), (2, 1), (12, 256)])
    call('ps', psize * players if atomic else psize, 1 if atomic else players)
    call('po', 16 if early else 256, players if early else 1)
    if old:
        call('hittype', 71, hits)
    calls([(2, 1), (3072, 1), (4, 2048), (4, 2048), (2, 1)])
    if atomic:
        call('spriteqamount', 2, 1)
    call('spriteq', 2, struct.unpack('<h', s.calls['spriteqamount'])[0] if atomic else 64)
    calls([(2, 1), (2, 64), (2, 64)])
    if version != '0.99':
        call('show2dsector', 1, 128)
    if atomic:
        calls([(1, tiles), (128, 1), (2, 1), (256, 1), (256, 1), (256, 1)])
    call('scriptflags', 1, scriptlen)
    call('script', 4, scriptlen)
    call('actors', 4, tiles)
    if not old:
        call('hitflags', 1, hits)
        call('hittype', 71, hits)
    calls([(4, 1), (2, 1), (2, 256)])
    call('animatecnt', 4, 1)
    call('animatesect', 2, 64)
    call('animateptr', 4, 64)
    calls([(4, 64), (4, 64), (1, 1), (2, 1)])
    if version == '0.99':
        calls([(2, 1)])
    elif early:
        calls([(4, 1), (4, 1)])
        if not old:
            calls([(1, 1)] * 3)
    else:
        calls([(2, 1), (4, 1), (4, 1), (4, 1), (1, 1), (4, 1), (4, 1), (4, 1), (2, 1), (1, 1)])
        if atomic:
            calls([(4, 1)] * 3 + [(2, 1), (2, 1), (32, 1), (1, 1), (512, 1), (4, 1), (2, 1), (4, 1)])
    assert offset == len(data), (offset, len(data))
    return s


def stored(data, size):
    """Encode one call with stored LZW blocks, preserving dfwrite boundaries."""
    if size > 16384:
        size = 1
    delta = bytearray(data)
    for i in range(size, len(data)):
        delta[i] = (data[i] - data[i - size]) & 255
    chunk = (16384 // size) * size
    return b''.join(struct.pack('<hHh', 4 + len(part), len(part), 0) + part
                    for i in range(0, len(data), chunk)
                    for part in [delta[i:i + chunk]])


def replace(save, fields):
    blocks = list(save.blocks)
    for name, (value, size) in sorted(fields.items(), key=lambda item: save.call_blocks[item[0]][0], reverse=True):
        first, count = save.call_blocks[name]
        blocks[first:first + count] = [stored(value, size)]
    return b''.join(blocks)


def main():
    p = argparse.ArgumentParser()
    for name in ('save', 'version', 'work'):
        p.add_argument('--' + name, required=True)
    target = p.add_mutually_exclusive_group(required=True)
    target.add_argument('--duke3d')
    target.add_argument('--codec')
    p.add_argument('--data')
    a = p.parse_args()
    if a.duke3d and not a.data:
        p.error('--duke3d requires --data')
    work = Path(a.work).resolve()
    work.mkdir(parents=True, exist_ok=True)
    for path in Path(a.data).iterdir() if a.data else ():
        if path.is_file() and path.suffix.lower() in ('.grp', '.con', '.dat', '.art'):
            dest = work / path.name.lower()
            if not dest.exists():
                dest.symlink_to(path.resolve())
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', BUILD_WINDOWED='1',
               HOME=str(work), USERPROFILE=str(work), DUKE3D_SAVE_DIR=str(work))
    original = Path(a.save).read_bytes()
    s = parse(original, a.version)
    cases = {'original': (original, 0),
             'truncated_header': (original[:3], 2),
             'truncated_middle': (original[:len(original) // 2], 2),
             'truncated_tail': (original[:-1], 2)}
    for value in (-1, 1, 0x7fffffff):
        ptrs = struct.pack('<i', value) + s.calls['animateptr'][4:]
        cases['animation_' + str(value)] = (replace(s, {'animatecnt': (struct.pack('<i', 1), 4), 'animateptr': (ptrs, 4)}), 2)
    # Stale entries beyond animatecnt must remain ignored.
    cases['unused_animation'] = (replace(s, {'animatecnt': (struct.pack('<i', 0), 4),
                                           'animateptr': (b'\xff' * 256, 4)}), 0)
    if a.version in ('1.4', '1.5'):
        cases['zero_sprite_queue'] = (replace(s, {'spriteqamount': (b'\0\0', 2),
                                                 'spriteq': (s.calls['spriteq'][:2], 2)}), 0)
    if a.version not in ('0.99', '1.0'):
        for value in (-4, 3, len(s.calls['script'])):
            v = struct.pack('<i', value)
            cases['actor_' + str(value)] = (replace(s, {'actors': (v + s.calls['actors'][4:], 4)}), 2)
            cases['script_' + str(value)] = (replace(s, {'scriptflags': (s.calls['scriptflags'][:1] + b'\1' + s.calls['scriptflags'][2:], 1),
                                                         'script': (s.calls['script'][:4] + v + s.calls['script'][8:], 4)}), 2)
            for bit, slot in enumerate((1, 4, 5)):
                hit = bytearray(s.calls['hittype'])
                hit[47 + slot * 4:51 + slot * 4] = v
                cases['hit_%d_%d' % (slot, value)] = (replace(s, {'hitflags': (bytes([1 << bit]) + s.calls['hitflags'][1:], 1),
                                                                  'hittype': (hit, 71)}), 2)
    if a.version not in ('0.99', '1.0'):
        end = len(s.calls['script'])
        cases['script_header_end'] = (replace(s, {'scriptflags': (b'\1' + s.calls['scriptflags'][1:], 1),
                                                 'script': (struct.pack('<i', end) + s.calls['script'][4:], 4)}), 0)
        cases['actor_near_end'] = (replace(s, {'actors': (struct.pack('<i', end - 16) + s.calls['actors'][4:], 4)}), 2)
        for bit, (slot, size) in enumerate(((1, 8), (4, 20), (5, 12))):
            hit = bytearray(s.calls['hittype'])
            hit[47 + slot * 4:51 + slot * 4] = struct.pack('<i', end - size + 4)
            cases['hit_near_end_%d' % slot] = (replace(s, {'hitflags': (bytes([1 << bit]) + s.calls['hitflags'][1:], 1),
                                                         'hittype': (hit, 71)}), 2)
    failures = []
    for name, (data, want) in cases.items():
        (work / 'check.sav').write_bytes(data)
        command = ([str(Path(a.duke3d).resolve()), '-loadverify', 'check.sav'] if a.duke3d
                   else [str(Path(a.codec).resolve()), 'check.sav'])
        r = subprocess.run(command, cwd=work, env=env,
                           capture_output=True, text=True, timeout=30)
        if r.returncode != want:
            failures.append(name)
            print('%s: exit %d, expected %d\n%s%s' % (name, r.returncode, want, r.stdout, r.stderr))
    if a.duke3d and a.version in ('0.99', '1.0', '1.1'):
        # The compiler's end marker describes a larger modified CON without
        # requiring a modified CON to boot the game for this warning check.
        modified = struct.pack('<i', 10241 * 4) + s.calls['script'][4:]
        (work / 'check.sav').write_bytes(replace(s, {'script': (modified, 4), 'scriptflags': (b'\0' + s.calls['scriptflags'][1:], 1)}))
        r = subprocess.run([str(Path(a.duke3d).resolve()), '-loadverify', 'check.sav', '-saveverify', 'out.sav'],
                           cwd=work, env=env, capture_output=True, text=True, timeout=30)
        if r.returncode != 0 or 'Warning: DOS saves store only 10240 script longs' not in r.stdout:
            failures.append('oversize_script_warning')
            print('missing oversize CON warning: %s%s' % (r.stdout, r.stderr))
    if failures:
        raise SystemExit('save hardening failed: ' + ', '.join(failures))
    print('save hardening passed: %d cases (%s)' % (len(cases), a.version))


if __name__ == '__main__':
    main()
