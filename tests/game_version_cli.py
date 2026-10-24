"""Exercise profile selection through the real executable, before game assets load."""
import argparse
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import threading
import zlib

p = argparse.ArgumentParser()
p.add_argument('--duke3d', required=True)
p.add_argument('--work', required=True)
p.add_argument('--fixture', action='append', default=[])
a = p.parse_args()
exe = Path(a.duke3d).resolve()
work = Path(a.work).resolve()
work.mkdir(parents=True, exist_ok=True)
empty = b'KenSilverman' + struct.pack('<I', 0)
crc = '%08x' % zlib.crc32(empty)
failures = []

def run(name, args, grp=None, binary=exe, cwd=None, selection_only=False):
    run_dir = cwd or work / name
    run_dir.mkdir(parents=True, exist_ok=True)
    home = run_dir / 'home'
    home.mkdir(exist_ok=True)
    if grp is not None:
        (run_dir / 'DUKE3D.GRP').write_bytes(grp)
    env = os.environ | {'SDL_VIDEODRIVER': 'dummy', 'SDL_AUDIODRIVER': 'dummy',
        'BUILD_WINDOWED': '1', 'HOME': str(home), 'USERPROFILE': str(home),
        'DUKE3D_SAVE_DIR': str(home)}
    command = [str(binary), *args]
    if selection_only:
        proc = subprocess.Popen(command, cwd=run_dir, env=env,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        lines = []
        def read_selection():
            for line in proc.stdout:
                lines.append(line)
                if 'Selected game profile:' in line:
                    proc.terminate()
                    break
        reader = threading.Thread(target=read_selection, daemon=True)
        reader.start()
        try:
            proc.wait(timeout=15)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        reader.join(timeout=2)
        return proc.returncode, ''.join(lines)
    r = subprocess.run(command, cwd=run_dir, env=env,
        capture_output=True, text=True, timeout=15)
    return r.returncode, r.stdout + r.stderr

def check(name, condition, output):
    if not condition:
        failures.append(name)
        print('FAIL %s\n%s' % (name, output))

def early_error(name, args, grp, message):
    code, out = run(name, args, grp)
    check(name, code != 0 and message.casefold() in out.casefold() and
        'Compiling:' not in out and "This client's ID" not in out, out)
    return out

for flag in ['--help', '-help', '?', '/?']:
    code, out = run('help_' + str(len(failures)) + flag.replace('/', '_').replace('?', 'q'), [flag])
    check('help without GRP ' + flag, code == 0 and '-gameversion' in out and
          '1.3d-shareware' in out and 'Compiling:' not in out, out)
for name, args in [('missing', ['-gameversion']), ('bad', ['-gameversion', '2.0']),
                   ('duplicate', ['-gameversion', '1.5', '-gameversion', '1.5'])]:
    early_error(name, args, None, '-gameversion')
out = early_error('unknown', [], empty, 'Unrecognized GRP')
check('unknown fingerprint diagnostic', '16' in out and crc in out.lower() and 'duke3d.grp' in out.lower(), out)
for token in ['0.99', '1.0', '1.1', '1.3d-shareware', '1.3d', '1.4', '1.5']:
    code, out = run('forced_' + token, ['-gameversion', token], empty, selection_only=True)
    check('forced unknown ' + token, 'Selected game profile: ' + token in out, out)
for i, bad in enumerate([b'', b'not a GRP', b'KenSilverman' + struct.pack('<I', 1)]):
    early_error('malformed_' + str(i), ['-gameversion', '1.5'], bad, 'Invalid GRP')
for flag in [['-map', 'custom.map'], ['custom.map'], ['/v2'], ['/l7']]:
    early_error('shareware_' + str(len(failures)) + flag[0].replace('/', '_'),
        ['-gameversion', '1.3d-shareware', *flag], empty, 'Shareware')

fixtures = [x.split('=', 1) for x in a.fixture]
for token, path in fixtures:
    target = work / ('known_' + token)
    target.mkdir(exist_ok=True)
    shutil.copyfile(path, target / 'DUKE3D.GRP')
    with open(path, 'rb') as grp:
        grp.seek(12)
        count = struct.unpack('<I', grp.read(4))[0]
        entries = [grp.read(16) for _ in range(count)]
        offset = 16 + count*16
        for entry in entries:
            name = entry[:12].split(b'\0')[0].decode('ascii')
            size = struct.unpack_from('<I', entry, 12)[0]
            if name.upper() in ('GAME.CON', 'DEFS.CON', 'USER.CON'):
                grp.seek(offset)
                (target / name.upper()).write_bytes(grp.read(size))
            offset += size
    code, out = run('known_' + token, ['-loadverify', 'missing.sav'], cwd=target)
    check('detected ' + token, 'Selected game profile: ' + token in out and '; detected ' in out, out)
    if token in ['0.99', '1.0', '1.1', '1.3d-shareware']:
        code, out = run('known_' + token, ['-gameversion', '1.5'], cwd=target)
        check('shareware full conflict ' + token, code != 0 and 'Shareware' in out and 'Compiling:' not in out, out)
    if token in ['1.3d', '1.5']:
        code, out = run('known_' + token, ['-gameversion', '1.4', '-loadverify', 'missing.sav'], cwd=target, selection_only=True)
        check('retail override logs both identities ' + token, 'Selected game profile: 1.4' in out and '; detected ' in out and '; forced' in out, out)
    if fixtures:
        shutil.copyfile(fixtures[-1][1], target / 'addon.grp')
        code, out = run('known_' + token, ['/gaddon.grp', '-loadverify', 'missing.sav'], cwd=target)
        check('addon keeps base profile ' + token, 'Selected game profile: ' + token in out and
              out.count('Selected game profile:') == 1, out)
    modified = work / ('modified_' + token)
    modified.mkdir(exist_ok=True)
    shutil.copyfile(path, modified / 'DUKE3D.GRP')
    for con in ('GAME.CON', 'DEFS.CON', 'USER.CON'):
        shutil.copyfile(target / con, modified / con)
    with (modified / 'DUKE3D.GRP').open('ab') as f:
        f.write(b'\0')
    code, out = run('modified_' + token, [], cwd=modified)
    check('modified requires override ' + token, code != 0 and 'Unrecognized GRP' in out and 'Compiling:' not in out, out)
    code, out = run('modified_' + token, ['-gameversion', token, '-loadverify', 'missing.sav'], cwd=modified)
    check('modified forced ' + token, 'Selected game profile: ' + token in out and 'Compiling:' in out, out)
if sys.platform == 'darwin':
    bundle = work / 'bundle'
    program_dir = bundle / 'Contents' / 'MacOS'
    program_dir.mkdir(parents=True, exist_ok=True)
    binary = program_dir / exe.name
    shutil.copy2(exe, binary)
    (bundle / 'DUKE3D.GRP').write_bytes(empty)
    code, out = run('outside_bundle', ['-gameversion', '1.3d'], binary=binary, selection_only=True)
    check('bundle working directory', 'Selected game profile: 1.3d' in out, out)
print('%d CLI checks failed' % len(failures))
sys.exit(bool(failures))
