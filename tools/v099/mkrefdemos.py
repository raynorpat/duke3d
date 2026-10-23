"""Regenerate beta E1 reference demos using original DOS playback as authority."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
HASHES = {
    'DUKE3D.EXE': '18c7f243e10e6953c298c24e4c798e41',
    'DUKE3D.GRP': '56b35e575eba7f16c0e19628bd6bd934',
    'GAME.CON': '90130f9a2f0b0ab42975f2602e1456ba',
    'DEFS.CON': '53a1e1568da0b9078e199cb312268717',
    'USER.CON': 'b2bf074f6af92d87393fbb8ef60986da',
}
FIELDS = ('TICS', 'X', 'Y', 'Z', 'ANG', 'HEALTH', 'RANDOMSEED')


def checksums(data):
    for name, expected in HASHES.items():
        got = hashlib.md5((data / name).read_bytes()).hexdigest()
        if got != expected:
            raise ValueError(f'{name}: MD5 {got}, expected {expected}')


def native(exe, work, env, args, label):
    p = subprocess.run([str(exe), *args], cwd=work, env=env,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       text=True, timeout=90)
    (work / f'{label}.log').write_text(p.stdout)
    if p.returncode:
        raise RuntimeError(f'{label} exited {p.returncode}; see {work}/{label}.log')
    tag = 'ROUTESTATE' if label == 'author' else 'DEMOSTATE'
    m = re.search(tag + r' ((?:-?\d+ ){6}-?\d+)', p.stdout)
    if not m or (label == 'author' and ' EOL 1' not in m.string[m.end():m.end()+8]):
        raise RuntimeError(f'{label} did not reach the exit: {work}/{label}.log')
    state = list(map(int, m[1].split()))
    state[-1] &= 0xffffffff
    return state


def dos(data, work, exe, demo, dosbox):
    for name in ('DUKE3D.GRP', 'GAME.CON', 'DEFS.CON', 'USER.CON', 'DUKE.RTS', 'DOS4GW.EXE'):
        (work / name).symlink_to(data / name)
    shutil.copyfile(data / 'DUKE3D.CFG', work / 'DUKE3D.CFG')
    shutil.copyfile(exe, work / 'DUKE3D.EXE')
    shutil.copyfile(demo, work / 'DEMO1.DMO')
    config = f'''[sdl]
output=surface
[dosbox]
machine=svga_s3
memsize=32
[cpu]
core=dynamic
cycles=max
[mixer]
nosound=true
[sblaster]
sbtype=sb16
sbbase=220
irq=5
dma=3
hdma=7
[speaker]
pcspeaker=false
[autoexec]
mount c {work}
c:
AUTOTYPE -w 3 -p 0.1 enter
DUKE3D.EXE
'''
    (work / 'run.conf').write_text(config)
    count = struct.unpack_from('<I', demo.read_bytes())[0]
    p = None
    try:
        with (work / 'dosbox.log').open('w') as log:
            p = subprocess.Popen([dosbox, '-conf', str(work / 'run.conf'), '-noconsole'],
                                 cwd=work, stdout=log, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + max(60, count / 26 + 60)
            capture = work / 'STATE.BIN'
            while time.monotonic() < deadline and p.poll() is None:
                if capture.exists() and capture.stat().st_size >= 181063:
                    time.sleep(0.2)
                    break
                time.sleep(0.2)
            else:
                raise RuntimeError(f'no complete DOS capture: {work}')
    finally:
        if p is not None and p.poll() is None:
            p.terminate()
            try:
                p.wait(timeout=5)
            except subprocess.TimeoutExpired:
                p.kill()
                p.wait()
        checksums(data)
    b = capture.read_bytes()
    remaining, seed = struct.unpack_from('<II', b)
    ps = 8 + 4096 * 44
    player = struct.unpack_from('<h', b, ps + 0x2bc)[0]
    if not 0 <= player < 4096 or remaining or not b[ps + 0x2dc] & 8:
        raise RuntimeError(f'DOS did not reach the exit: {work}')
    sp = 8 + player * 44
    return [count, *struct.unpack_from('<iii', b, sp),
            struct.unpack_from('<h', b, sp + 28)[0],
            struct.unpack_from('<h', b, sp + 42)[0], seed]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('data', type=Path, help='original beta data directory')
    ap.add_argument('build', type=Path, help='0.99 build with DUKE3D_ROUTEBOT=ON')
    ap.add_argument('levels', type=int, choices=range(1, 7), nargs='*', default=list(range(1, 7)))
    ap.add_argument('--dosbox', default=shutil.which('dosbox-x'))
    a = ap.parse_args()
    if not a.dosbox:
        ap.error('DOSBox-X not found; supply --dosbox')
    data, exe = a.data.resolve(), a.build.resolve() / 'bin/duke3d'
    checksums(data)
    work = Path(tempfile.mkdtemp(prefix='v099-ref-', dir='/tmp'))
    print(f'Scratch: {work}', flush=True)
    flat = work / 'beta099.bin'
    subprocess.run([sys.executable, str(ROOT / 'tools/v13d/le_flatten.py'),
                    str(data / 'DUKE3D.EXE'), '2a50', str(flat)], check=True)
    for level in a.levels:
        route = ROOT / f'tools/v099/routes/e1l{level}.route'
        god = any(line.strip() == '# DEMOGOD' for line in route.read_text().splitlines())
        run = work / f'e1l{level}'
        run.mkdir()
        home = run / 'home'
        home.mkdir()
        for name in ('DUKE3D.GRP', 'GAME.CON', 'DEFS.CON', 'USER.CON'):
            (run / ('duke3d.grp' if name.endswith('GRP') else name)).symlink_to(data / name)
        env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy',
                   HOME=str(home), USERPROFILE=str(home), DUKE3D_SAVE_DIR=str(home))
        options = ['-demogod'] if god else []
        author = native(exe, run, env, ['/v1', f'/l{level}', '/s3', '-routebot', str(route), '-r', *options], 'author')
        replay = native(exe, run, env, ['-demoverify', '1', *options], 'replay')
        if author != replay:
            raise RuntimeError(f'author/replay mismatch: {author} / {replay}')
        patched = run / 'capture.exe'
        subprocess.run([sys.executable, str(ROOT / 'tools/v13d/mkcapture.py'), 'sw099',
                        str(data / 'DUKE3D.EXE'), str(patched), '1', *(['demogod'] if god else [])],
                       env=dict(os.environ, V13D_FLAT=str(flat)), check=True)
        doswork = run / 'dos'
        doswork.mkdir()
        measured = dos(data, doswork, patched, run / 'demo1.dmo', a.dosbox)
        if measured != replay:
            raise RuntimeError(f'DOS/native mismatch: {measured} / {replay}; scratch {run}')
        refs = ROOT / 'tests/data/v13d_demo_refs.cmake'
        text = refs.read_text()
        prefix = f'V13D_DEMO_SW099_E1L{level}_'
        values = dict(zip(FIELDS, measured), DEMOGOD='ON' if god else 'OFF')
        for key, value in values.items():
            entry = f'set({prefix}{key} {value})'
            pattern = r'^set\(' + prefix + key + r' [^\n]*\)$'
            if re.search(pattern, text, flags=re.M):
                text = re.sub(pattern, entry, text, flags=re.M)
            else:
                text += entry + '\n'
        (data / 'refdemos').mkdir(exist_ok=True)
        shutil.copyfile(run / 'demo1.dmo', data / f'refdemos/E1L{level}.DMO')
        refs.write_text(text)
        print(f'E1L{level} DOS/native exact: {measured}; installed reference', flush=True)
    checksums(data)


if __name__ == '__main__':
    main()
