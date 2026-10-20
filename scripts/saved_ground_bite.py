"""Mutate private source copies; seeded occupancy/material regressions must fail."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    build = Path(tempfile.mkdtemp(prefix='sav-ground-bite-'))
    original = (ROOT / 'game_lab/original_ground.cpp').read_text()
    mutations = [
        ('occupancy', 'if(!rebuild)working.image.assign', 'if(true)working.image.assign'),
        ('signed-material', 'const int value=int8_t((*material)[cell]);',
         'const int value=(*material)[cell];'),
        ('corner', 'column==0&&row==side-1)?0x40:0));\n        rebuild->subTileWritten',
         'column==0&&row==0)?0x40:0));\n        rebuild->subTileWritten'),
    ]
    env = dict(os.environ)
    env.pop('EMPEROR_GAMEDATA_ROOT', None)
    for name, before, after in mutations:
        if original.count(before) != 1:
            raise ValueError('mutation must address one exact source site: ' + name)
        source = build / (name + '.cpp')
        source.write_text(original.replace(before, after))
        binary = build / name
        sources = ['original_saved_ground_tests.cpp', 'app/map_surface.cpp', 'map_reference.cpp',
                   'original_map.cpp', 'original_save.cpp', 'original_pak.cpp', 'original_terrain.cpp',
                   'resource_root.cpp', 'sg3.cpp']
        subprocess.run(['/usr/bin/clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-I', str(ROOT / 'game_lab'), str(source),
                        *[str(ROOT / 'game_lab' / item) for item in sources],
                        '-framework', 'CoreGraphics', '-framework', 'CoreFoundation', '-lz',
                        '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], env=env, capture_output=True, text=True)
        print(name, 'exit', result.returncode, result.stderr.strip(), flush=True)
        if result.returncode != 1:
            raise RuntimeError('mutation survived: ' + name)
    print('3/3 private mutations rejected; production source unchanged')


if __name__ == '__main__':
    main()
