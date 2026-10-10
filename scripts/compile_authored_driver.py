"""Compile the existing authored C driver to an object; never link or run it."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
SOURCES = (
    'legacy/src/stage82/tools/native_driver.c',
    'legacy/src/stage82/tools/runtime.h',
    'legacy/src/stage80/tools/sparse_memory.h',
    'legacy/src/stage79/tools/guest_memory.h',
    'legacy/src/stage79/tools/thread_native.h',
    'legacy/src/stage78/tools/guarded_helpers.h',
    'legacy/src/stage77/tools/heap_native.h',
)
PROVENANCE = (
    'legacy/src/stage82/tools/build_drivers.py',
    'legacy/src/stage82/tools/fixture.py',
)
REQUIRED_UNRESOLVED = {
    'entry': ('s82_entry', 's82_completion'),
    'return_bindings': ('stage72_return_bindings', 'stage72_return_bindings_count'),
    'target_bindings': ('stage77_structural_code_bindings',),
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def unique_keys(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('duplicate scope field: ' + key)
        result[key] = value
    return result


def read_scope(path):
    raw = path.read_bytes()
    if len(raw) > 8192:
        raise ValueError('scope exceeds 8192 bytes')
    value = json.loads(raw, object_pairs_hook=unique_keys)
    fields = {'schema', 'kind', 'entry', 'return', 'allowed_pcs', 'target_count'}
    if type(value) is not dict or set(value) != fields:
        raise ValueError('scope requires exactly: ' + ', '.join(sorted(fields)))
    if value['schema'] != 'AUTHORED_NATIVE_DRIVER_COMPILE_SCOPE_V1' or value['kind'] != 'authored_compile_only':
        raise ValueError('only authored compile scope is supported')
    pcs = value['allowed_pcs']
    if type(pcs) is not list or not 1 <= len(pcs) <= 4096:
        raise ValueError('allowed_pcs must be a nonempty bounded list')
    for pc in [value['entry'], value['return'], *pcs]:
        if type(pc) is not int or not 0xa0000000 <= pc < 0xa0001000:
            raise ValueError('PC must be an integer in the existing authored fixture range')
    if pcs != sorted(set(pcs)) or value['entry'] not in pcs or value['return'] in pcs:
        raise ValueError('PCs must be sorted and unique, include entry and exclude return')
    if type(value['target_count']) is not int or value['target_count'] != 0:
        raise ValueError('this authored fixture requires target_count=0; no target table is supplied')
    return raw, value


def command(argv, work, label):
    env = {'PATH': '/usr/bin:/bin:/usr/sbin:/sbin', 'LC_ALL': 'C', 'TMPDIR': str(work / 'tmp')}
    started = time.monotonic()
    proc = subprocess.Popen(argv, cwd=work, env=env, start_new_session=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    timed_out = False
    try:
        stdout, stderr = proc.communicate(timeout=60)
    except subprocess.TimeoutExpired:
        timed_out = True
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        stdout, stderr = proc.communicate()
    elapsed = time.monotonic() - started
    (work / (label + '.stdout.log')).write_bytes(stdout)
    (work / (label + '.stderr.log')).write_bytes(stderr)
    return dict(label=label, argv=argv, exit_code=proc.returncode,
                timed_out=timed_out, elapsed_seconds=elapsed, timeout_seconds=60), stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--scope', required=True, type=Path,
                        help='explicit authored compile scope JSON; there is no default')
    parser.add_argument('--output-root', type=Path, default=Path(tempfile.gettempdir()),
                        help='existing directory; a new child build directory is always created')
    args = parser.parse_args()
    # Required inputs are validated before creating a build directory or invoking tools.
    scope_raw, scope = read_scope(args.scope)
    source_raw = {name: (ROOT / name).read_bytes() for name in SOURCES + PROVENANCE}
    controller_raw = Path(__file__).read_bytes()
    output_root = args.output_root.resolve(strict=True)
    if not output_root.is_dir():
        raise ValueError('output-root must be an existing directory')
    work = Path(tempfile.mkdtemp(prefix='authored-driver-compile-', dir=output_root))
    (work / 'tmp').mkdir()
    for name, raw in source_raw.items():
        dest = work / 'input' / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(raw)
    (work / 'scope.json').write_bytes(scope_raw)
    generated = work / 'generated'
    generated.mkdir()
    header = ('#define S82_MODE 2\n'
              '#define S82_ENTRY ' + str(scope['entry']) + 'u\n'
              '#define S82_RETURN ' + str(scope['return']) + 'u\n'
              '#define S82_TARGET_COUNT ' + str(scope['target_count']) + 'u\n'
              'static const uint32_t s82_allowed[]={' +
              ','.join(str(pc) + 'u' for pc in scope['allowed_pcs']) + '};\n')
    (generated / 'build_scope.h').write_text(header)
    includes = ['generated'] + ['input/legacy/src/' + stage + '/tools'
                               for stage in ('stage82', 'stage80', 'stage79', 'stage78', 'stage77')]
    argv = ['/usr/bin/clang', '-std=c11', '-fno-modules', '-O1', '-g', '-fdebug-compilation-dir=.',
            '-Wall', '-Wextra', '-Werror', '-Wno-logical-op-parentheses']
    argv += ['-I' + directory for directory in includes]
    argv += ['-MMD', '-MF', 'native_driver.d', '-c', 'input/' + SOURCES[0], '-o', 'native_driver.o']
    process, _ = command(argv, work, 'compile')
    processes = [process]
    symbols = []
    groups = {}
    if process['exit_code'] == 0 and not process['timed_out']:
        process, stdout = command(['/usr/bin/nm', '-u', 'native_driver.o'], work, 'nm')
        processes.append(process)
        symbols = sorted(line.split()[-1] for line in stdout.decode().splitlines() if line.split())
        for group, required in REQUIRED_UNRESOLVED.items():
            groups[group] = [symbol for symbol in symbols if symbol.lstrip('_') in required]
    unresolved_checked = all({s.lstrip('_') for s in groups.get(group, [])} == set(required)
                             for group, required in REQUIRED_UNRESOLVED.items())
    stable = all((ROOT / name).read_bytes() == raw for name, raw in source_raw.items())
    stable = stable and args.scope.read_bytes() == scope_raw and Path(__file__).read_bytes() == controller_raw
    passed = stable and unresolved_checked and all(p['exit_code'] == 0 and not p['timed_out']
                                                   and p['elapsed_seconds'] < 60 for p in processes)
    obj = work / 'native_driver.o'
    result = dict(schema='AUTHORED_NATIVE_DRIVER_OBJECT_CHECK_V1', passed=passed,
        status='OBJECT_COMPILED_UNLINKED_UNEXECUTED' if passed else 'CHECK_FAILED',
        sources_sha256={name: sha(raw) for name, raw in source_raw.items()},
        controller_sha256=sha(controller_raw), scope_sha256=sha(scope_raw), scope=scope,
        generated_header_sha256=sha(header.encode()), object_sha256=sha(obj.read_bytes()) if obj.exists() else None,
        unresolved_symbols=symbols, required_unresolved_groups=groups,
        required_unresolved_checked=unresolved_checked, source_stable=stable, processes=processes,
        linked=False, executed=False, game_IR_compiled=False, original_binding_tables_supplied=False,
        archived_python_imported_or_executed=False, game_assets_read=False,
        meaning='one authored C translation unit compiles; entry and binding definitions remain missing from this build')
    (work / 'result.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps(dict(report=str(work), passed=passed, linked=False, executed=False,
                          required_unresolved_groups=groups), ensure_ascii=False))
    if not passed:
        raise ValueError('object check failed; see result.json and process logs')


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, json.JSONDecodeError) as error:
        raise SystemExit('authored driver compile: ' + str(error))
