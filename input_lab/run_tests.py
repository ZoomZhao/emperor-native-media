#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Own-window input checks only. Never launches a game or reads GameData."""
import argparse
import json
import pathlib
import plistlib
import subprocess
import sys
import tempfile

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    parser.add_argument('--window', action='store_true', help='opt in to own AppKit window/input/capture test')
    parser.add_argument('--build-dir', type=pathlib.Path)
    parser.add_argument('--request', type=pathlib.Path, help='submit one existing-window click instead of running tests')
    parser.add_argument('--inspect', action='store_true', help='with --request, read guard metadata without sending input')
    args = parser.parse_args()
    if args.inspect and not args.request:
        parser.error('--inspect requires --request')
    if args.window and args.request:
        parser.error('--window and --request are mutually exclusive')
    source = pathlib.Path(__file__).resolve().parent
    build = args.build_dir.resolve() if args.build_dir else pathlib.Path(tempfile.mkdtemp(prefix='authored-input-'))
    build.mkdir(parents=True, exist_ok=True)
    common = ['/usr/bin/clang++', '-std=c++17', '-fobjc-arc', '-mmacosx-version-min=15.0',
              '-Wall', '-Wextra', '-Wpedantic', '-Werror']
    common += ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    def compile_source(name, output, frameworks):
        command = common + [str(source / name), '-o', str(output)]
        for framework in frameworks:
            command += ['-framework', framework]
        subprocess.run(command, check=True, timeout=60)
    frameworks = ['AppKit', 'ApplicationServices', 'ScreenCaptureKit']
    compile_source('cli.mm', build / 'scoped-click', frameworks)
    if args.request:
        subprocess.run([str(build / 'scoped-click')] + (['--inspect'] if args.inspect else []) + [str(args.request.resolve())], check=True, timeout=10)
        return
    compile_source('guard_tests.mm', build / 'guard-tests', frameworks)
    result = subprocess.run([str(build / 'guard-tests')], check=True, capture_output=True, text=True, timeout=10)
    request = {'pid':123, 'uid':501, 'birth_seconds':42, 'birth_micros':1234,
               'executable':'/synthetic/Fixture', 'window_id':77,
               'frame_x':220, 'frame_y':242, 'frame_width':640, 'frame_height':448,
               'normalized_x':0.484375, 'normalized_y':247/448, 'deadline_unix_seconds':1100}
    invalid = [(dict(request, pid=True), 'numeric request field required'),
               (dict(request, pid=2147483648), 'integer request field out of range'),
               (dict(request, window_id=-1), 'integer request field out of range'),
               (dict(request, frame_width='640'), 'numeric request field required'),
               ([], 'JSON request object required')]
    for index, (value, reason) in enumerate(invalid):
        path = build / ('invalid-request-%d.json' % index)
        path.write_text(json.dumps(value), encoding='utf-8')
        check = subprocess.run([str(build / 'scoped-click'), str(path)], capture_output=True, text=True, timeout=10)
        if check.returncode != 1 or check.stdout or check.stderr.strip() != reason:
            raise RuntimeError('CLI invalid request rejection mismatch')
    summary = json.loads(result.stdout)
    summary['CLI_parser_rejections'] = len(invalid)
    print(json.dumps(summary))
    (build / 'guard-result.json').write_text(json.dumps(summary, indent=2)+'\n', encoding='utf-8')
    if args.window:
        bundle = build / 'AuthoredInputFixture.app' / 'Contents'
        executable = bundle / 'MacOS' / 'Fixture'
        executable.parent.mkdir(parents=True, exist_ok=True)
        with (bundle / 'Info.plist').open('wb') as handle:
            plistlib.dump({'CFBundleExecutable':'Fixture', 'CFBundleIdentifier':'org.emperor-native-media.authored-input',
                          'CFBundleName':'AuthoredInputFixture', 'CFBundlePackageType':'APPL',
                          'CFBundleVersion':'1', 'LSMinimumSystemVersion':'15.0', 'NSHighResolutionCapable':True}, handle)
        compile_source('fixture.mm', executable, frameworks)
        subprocess.run(['/usr/bin/codesign', '--force', '--sign', '-', str(bundle.parent)], check=True, timeout=10)
        compile_source('integration_tests.mm', build / 'window-tests', frameworks + ['ImageIO'])
        evidence = pathlib.Path(tempfile.mkdtemp(prefix='own-window-', dir=str(build)))
        subprocess.run([str(build / 'window-tests'), str(executable), str(evidence), str(build / 'scoped-click')], check=True, timeout=70)
        print(json.dumps({'evidence':str(evidence), 'mode':args.mode}))
    print(json.dumps({'build_dir':str(build), 'original_game_launches':0}))

if __name__ == '__main__':
    try:
        main()
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
