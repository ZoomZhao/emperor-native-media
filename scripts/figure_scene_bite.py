"""Private figure mutations must fail authored pixel/wire/index regressions."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    work = Path(tempfile.mkdtemp(prefix='figure-scene-bite-'))
    mutations = [
        ('index', 'original_building_scene.hpp', '(word&0x3fff)+1;', '(word&0x3fff)+201;'),
        ('position', 'original_building_scene.hpp', 'ox-s.animationOffsetX,oy-s.animationOffsetY', 'ox+s.animationOffsetX,oy-s.animationOffsetY'),
        ('order', 'original_building_scene.hpp', 'return a.depth<b.depth;', 'return b.depth<a.depth;'),
        ('stride', 'original_figure.hpp', 'base+frame*directions+direction;', 'base+direction*frames+frame;'),
        ('wire', 'original_figure.hpp', 'f.action=r[64];', 'f.action=r[65];'),
        ('selector', 'original_figure_image.hpp', 'code=f.subtype?0x4c13:0x4c15;', 'code=f.subtype?0x4c15:0x4c13;'),
    ]
    env = dict(os.environ)
    env.pop('EMPEROR_GAMEDATA_ROOT', None)
    for name, header, before, after in mutations:
        private = work / name
        lab = private / 'game_lab'
        for path in (ROOT / 'game_lab').rglob('*'):
            if path.suffix not in ('.hpp', '.h', '.cpp', '.mm') or 'build' in path.relative_to(ROOT / 'game_lab').parts:
                continue
            target = lab / path.relative_to(ROOT / 'game_lab')
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, target)
        layout = private / 'file_io/include/city19_figure7_layout.hpp'
        layout.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / 'file_io/include/city19_figure7_layout.hpp', layout)
        original = (lab / header).read_text()
        if original.count(before) != 1:
            raise ValueError('mutation must address exactly one site: ' + name)
        (lab / header).write_text(original.replace(before, after))
        units = ['original_figure_tests.cpp', 'app/map_surface.cpp', 'map_reference.cpp',
                 'original_map.cpp', 'original_save.cpp', 'original_pak.cpp', 'original_terrain.cpp',
                 'original_ground.cpp', 'resource_root.cpp', 'sg3.cpp']
        binary = private / 'test'
        subprocess.run(['/usr/bin/clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                        *[str(lab / s) for s in units], '-framework', 'CoreGraphics',
                        '-framework', 'CoreFoundation', '-lz', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], env=env, capture_output=True, text=True)
        print(name, 'exit', result.returncode, result.stderr.strip(), flush=True)
        if result.returncode != 1:
            raise RuntimeError('mutation survived: ' + name)
    print('6/6 private figure mutations rejected; production source unchanged')


if __name__ == '__main__':
    main()
