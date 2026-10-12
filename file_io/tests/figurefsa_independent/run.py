#!/usr/bin/env python3
"""One explicit source root; finite host-only Release/UBSan, actual dependencies hashed."""
import argparse, datetime, hashlib, json, os, pathlib, subprocess, time

ap = argparse.ArgumentParser()
ap.add_argument('--source-root', required=True, type=pathlib.Path)
ap.add_argument('--output', required=True, type=pathlib.Path)
ap.add_argument('--modes', default='release,ubsan', choices=['release,ubsan', 'release', 'ubsan'])
args = ap.parse_args()
root = args.source_root.resolve()
out = args.output.resolve()
out.mkdir(parents=True, exist_ok=False)
source = pathlib.Path(__file__).resolve().with_name('properties.cpp')
ledger = source.with_name('expectations.json')
started = time.monotonic()
deadline = started + 180
records, results = [], []

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def run(argv, label, cap):
    remaining = deadline - time.monotonic()
    if remaining <= 0:
        raise TimeoutError('overall 180-second budget exhausted')
    t = time.monotonic()
    try:
        p = subprocess.run(argv, capture_output=True, cwd=root,
                           timeout=min(cap, remaining), env=os.environ.copy())
        stdout, stderr, code = p.stdout, p.stderr, p.returncode
    except subprocess.TimeoutExpired as e:
        stdout, stderr, code = e.stdout or b'', e.stderr or b'', 124
    (out / (label + '.stdout')).write_bytes(stdout)
    (out / (label + '.stderr')).write_bytes(stderr)
    records.append(dict(argv=argv, label=label, exit_code=code,
                        seconds=time.monotonic() - t,
                        stdout_sha256=hashlib.sha256(stdout).hexdigest(),
                        stderr_sha256=hashlib.sha256(stderr).hexdigest()))
    return code, stdout, stderr

failure = None
try:
    code, sdk, _ = run(['/usr/bin/xcrun', '--sdk', 'macosx', '--show-sdk-path'], 'sdk', 10)
    if code:
        raise RuntimeError('SDK lookup failed')
    modes = {'release': ['-O2'], 'ubsan': ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=undefined']}
    for mode in args.modes.split(','):
        exe, deps = out / ('properties-' + mode), out / (mode + '.d')
        cmd = ['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
               *modes[mode], '-isysroot', sdk.decode().strip(),
               '-I', str(root / 'file_io/include'), '-MMD', '-MF', str(deps),
               str(source), '-o', str(exe)]
        code, _, _ = run(cmd, mode + '-compile', 60)
        result = dict(mode=mode, compile_exit=code)
        results.append(result)
        if code:
            continue
        # Parse our generated depfile, including Clang's escaped spaces, and hash
        # the exact headers selected by quoted includes and search precedence.
        import shlex
        deptext = deps.read_text().replace('\\\n', ' ')
        paths = [pathlib.Path(x).resolve() for x in shlex.split(deptext.split(':', 1)[1])]
        result['compiled_dependency_sha256'] = {str(p): sha(p) for p in paths}
        code, stdout, stderr = run([str(exe)], mode + '-test', 10)
        result.update(exit_code=code, stderr_empty=not stderr)
        try:
            result['report'] = json.loads(stdout)
        except (ValueError, UnicodeDecodeError):
            result['report'] = None
except Exception as e:
    failure = str(e)
ok = failure is None and len(results) == len(args.modes.split(',')) and all(
    r.get('exit_code') == 0 and r.get('stderr_empty') and (r.get('report') or {}).get('failed') == 0 for r in results)
report = dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(), passed=ok,
              source_root=str(root), results=results, commands=records, failure=failure,
              test_sha256=sha(source), runner_sha256=sha(pathlib.Path(__file__).resolve()),
              expectation_sha256=sha(ledger), elapsed_seconds=time.monotonic()-started,
              overall_seconds_limit=180, compile_seconds_limit=60, test_seconds_limit=10,
              synthetic_input_only=True, original_executed=False, actual_payload_read=False,
              verifies_original_instance=False, new_actual_bytes=0)
(out / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(dict(passed=ok, results=results, output=str(out)), indent=2))
raise SystemExit(0 if ok else 1)
