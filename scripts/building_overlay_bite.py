"""Private copied-header mutations must fail authored object placement/order tests."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    build = Path(tempfile.mkdtemp(prefix='building-overlay-bite-'))
    source = ROOT / 'game_lab'
    original = (source / 'original_building_scene.hpp').read_text()
    mutations = [
        ('anchor', 'original_building_scene.hpp', 'object.offsetX-39,object.offsetY-20', 'object.offsetX,object.offsetY'),
        ('order', 'original_building_scene.hpp', 'return a.depth<b.depth;', 'return b.depth<a.depth;'),
        ('role', 'original_building_scene.hpp', 'variant[12]={0,1,2,3,5,5,4,4,7,7,6,6}',
         'variant[12]={0,1,2,3,5,5,4,4,6,6,6,6}'),
        ('animation-tick', 'original_building_animation.hpp', '20u*base.animationSpeedId', '40u*base.animationSpeedId'),
        ('animation-gate', 'original_building_animation.hpp', 'if(a.suppress)return false;', 'if(a.suppress)return true;'),
        ('animation-offset', 'original_building_animation.hpp', 'int32_t(base.animationOffsetX),', 'int32_t(frame.animationOffsetX),'),
    ]
    env = dict(os.environ)
    env.pop('EMPEROR_GAMEDATA_ROOT', None)
    for name, header, before, after in mutations:
        original = (source / header).read_text()
        if original.count(before) != 1:
            raise ValueError('mutation must address exactly one site: ' + name)
        private = build / name
        private.mkdir()
        # All quoted includes resolve within this private copy; production headers
        # cannot enter once via the original path and again via the copied path.
        for path in source.rglob('*'):
            if path.suffix not in ('.hpp', '.h', '.cpp', '.mm') or 'build' in path.relative_to(source).parts:
                continue
            target = private / path.relative_to(source)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(path.read_bytes())
        (private / header).write_text(original.replace(before, after))
        binary = private / 'test'
        units = ['original_building_scene_tests.cpp', 'app/map_surface.cpp', 'map_reference.cpp',
                 'original_map.cpp', 'original_save.cpp', 'original_pak.cpp', 'original_terrain.cpp',
                 'original_ground.cpp', 'resource_root.cpp', 'sg3.cpp']
        subprocess.run(['/usr/bin/clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                        *[str(private / item) for item in units], '-framework', 'CoreGraphics',
                        '-framework', 'CoreFoundation', '-lz', '-o', str(binary)], check=True)
        fixtures = private / 'fixtures'
        fixtures.mkdir()
        result = subprocess.run([str(binary), str(fixtures)], env=env, capture_output=True, text=True)
        print(name, 'exit', result.returncode, result.stderr.strip(), flush=True)
        if result.returncode != 1:
            raise RuntimeError('mutation survived: ' + name)
    print('6/6 private overlay/animation mutations rejected; production source unchanged')


if __name__ == '__main__':
    main()
