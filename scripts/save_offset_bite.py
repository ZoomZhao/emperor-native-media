"""Private SAV19 offset mutations must fail the literal read-order fixtures."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    build = Path(tempfile.mkdtemp(prefix='sav19-offset-bite-'))
    mutations = [
        ('view', 'original_save.cpp', 'base+original_save19::savedView)',
         'base+original_save19::savedView-8000)'),
        ('scalar-ac', 'original_save.cpp', 'base+original_save19::g101D0AC)',
         'base+original_save19::g101D0AC-8000)'),
        ('scalar-b0', 'original_save.cpp', 'base+original_save19::g101D0B0)',
         'base+original_save19::g101D0B0-8000)'),
        ('rng', 'original_figure.hpp', 'terrain+randomFirst)', 'terrain+randomFirst-8000)'),
        ('record-width', 'original_building.hpp', 'recordBytes=2+173', 'recordBytes=2+93'),
    ]
    env = dict(os.environ)
    env.pop('EMPEROR_GAMEDATA_ROOT', None)
    units = ['original_map.cpp', 'original_pak.cpp', 'original_terrain.cpp',
             'original_save.cpp', 'resource_root.cpp', 'original_save_tests.cpp']
    for name, filename, before, after in mutations:
        private = build / name
        for folder in ('game_lab', 'file_io/include'):
            for source in (ROOT / folder).rglob('*'):
                if source.suffix not in ('.hpp', '.h', '.cpp') or 'build' in source.relative_to(ROOT / folder).parts:
                    continue
                target = private / source.relative_to(ROOT)
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(source.read_bytes())
        changed = private / 'game_lab' / filename
        text = changed.read_text()
        expected = 2 if name == 'view' else 1
        if text.count(before) != expected:
            raise ValueError('mutation site count changed: ' + name)
        changed.write_text(text.replace(before, after))
        binary = private / 'test'
        subprocess.run(['/usr/bin/clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                        *[str(private / 'game_lab' / unit) for unit in units], '-lz', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], env=env, capture_output=True, text=True)
        print(name, 'exit', result.returncode, result.stderr.strip(), flush=True)
        if result.returncode != 1:
            raise RuntimeError('mutation survived: ' + name)
    print('5/5 private SAV19 offset mutations rejected; production source unchanged')


if __name__ == '__main__':
    main()
