#!/usr/bin/env python3
"""Compile only new authored static ABI description/validation code; no legacy/runtime/game inputs."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True, help='New absolute task-owned evidence directory')
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    args = parser.parse_args()
    if not args.output.is_absolute() or args.output.exists():
        parser.error('--output must be a new absolute directory')
    compiler, cpp = shutil.which('clang'), shutil.which('clang++')
    if not compiler or not cpp:
        parser.error('Existing clang/clang++ required; no software will be installed')
    root = Path(__file__).resolve().parents[1]
    own = ['original_bridge/include/scheduler_shape.h', 'original_bridge/src/scheduler_shape.c',
           'original_bridge/tests/scheduler_shape_tests.c', 'original_bridge/include/static_abi.h']
    captured = {path: (root / path).read_bytes() for path in own}
    for path, content in captured.items():
        if (root / path).read_bytes() != content:
            parser.error('Authored source changed during capture: ' + path)
    args.output.mkdir(parents=True)
    sources = args.output / 'sources'
    rows = []
    for path, content in captured.items():
        target = sources / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(content)
        rows.append({'path': path, 'sha256': sha(target)})
    flags = ['-Wall', '-Wextra', '-Werror', '-pedantic']
    flags += ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    report = {'mode': args.mode, 'source_files': rows, 'runner_sha256': sha(Path(__file__).resolve()),
              'scope': 'Only newly authored source, system standard library and synthetic descriptor fixtures. No original/derived code, legacy headers, guest bytes or provider loaded.',
              'runtime_bindings': 0, 'per_process_timeout_seconds': 60, 'steps': []}

    def run(command, name):
        start = time.monotonic()
        try:
            result = subprocess.run(command, text=True, capture_output=True, timeout=60)
            output, code = result.stdout + result.stderr, result.returncode
        except subprocess.TimeoutExpired as exc:
            output = 'TIMEOUT after 60 seconds\n' + (exc.stdout or b'').decode() + (exc.stderr or b'').decode()
            code = 124
        log = args.output / (name + '.txt')
        log.write_text(output)
        report['steps'].append({'name': name, 'command': command, 'exit_code': code,
                                'seconds': round(time.monotonic() - start, 3), 'log_sha256': sha(log)})
        print(output, end='')
        return code == 0

    include = sources / 'original_bridge/include'
    standalone = sources / 'standalone.cpp'
    standalone.write_text('#include "scheduler_shape.h"\nint main(){return sizeof(OAbiInput)>0?0:1;}\n')
    report['standalone_sha256'] = sha(standalone)
    ok = run([cpp, '-std=c++17', *flags, '-I', str(include), '-fsyntax-only', str(standalone)], 'cxx-header-compile')
    binary = args.output / 'scheduler-shape-tests'
    if ok:
        ok = run([compiler, '-std=c11', *flags, '-I', str(include), str(sources / own[1]),
                  str(sources / own[2]), '-o', str(binary)], 'compile-own-validator')
    if ok:
        report['binary_sha256'] = sha(binary)
        ok = run([str(binary)], 'synthetic-descriptor-tests')
    report['passed'] = ok
    (args.output / 'result.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
    print(('PASS' if ok else 'FAIL') + ' receipt: ' + str(args.output / 'result.json'))
    return 0 if ok else 1


if __name__ == '__main__':
    raise SystemExit(main())
