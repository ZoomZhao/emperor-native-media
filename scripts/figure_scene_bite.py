"""Private figure mutations must fail authored pixel/wire/index regressions."""
from pathlib import Path
import argparse
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--only', help='Run one named mutation')
    args = parser.parse_args()
    work = Path(tempfile.mkdtemp(prefix='figure-scene-bite-'))
    mutations = [
        ('index', 'original_building_scene.hpp', '(word&0x3fff)+1;', '(word&0x3fff)+201;'),
        ('position', 'original_building_scene.hpp', 'ox-s.animationOffsetX,oy-s.animationOffsetY', 'ox+s.animationOffsetX,oy-s.animationOffsetY'),
        ('order', 'original_building_scene.hpp', 'return a.depth<b.depth;', 'return b.depth<a.depth;'),
        ('stride', 'original_figure.hpp', 'base+frame*directions+direction;', 'base+direction*frames+frame;'),
        ('wire', 'original_figure.hpp', 'f.action=r[64];', 'f.action=r[65];'),
        ('selector', 'original_figure_image.hpp', 'code=f.subtype?0x4c13:0x4c15;', 'code=f.subtype?0x4c15:0x4c13;'),
        ('leader', 'original_figure_image.hpp', 'leader->amount>400;', 'leader->amount>=400;'),
        ('attack', 'original_figure_image.hpp', 'code=distance>2?0x4ebf:0x4ec0;', 'code=distance>3?0x4ebf:0x4ec0;'),
        ('cargo', 'original_figure_image.hpp', 'goods->amount<=200?0:', 'goods->amount<200?0:'),
        ('first_tick', 'original_figure_image.hpp', 'f.frame=uint8_t(f.frame+1);', 'f.frame=uint8_t(f.frame);'),
        ('post_load', 'original_figure_image.hpp', 'out.images=std::make_pair(0u,0u);', 'out.images=std::make_pair(0u,1u);'),
        ('zero_gate', 'original_building_scene.hpp', 'if(!f.image){', 'if(!f.image&&source==OriginalFigureRenderSource::Saved){'),
    ]
    if args.only:
        mutations = [mutation for mutation in mutations if mutation[0] == args.only]
        if not mutations:
            parser.error('unknown mutation: ' + args.only)
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
        for layout_name in ['city19_figure7_layout.hpp', 'city19_goods1_layout.hpp',
                     'city19_empire2_layout.hpp', 'city19_empire_grid15_layout.hpp']:
            layout = private / 'file_io/include' / layout_name
            layout.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / 'file_io/include' / layout_name, layout)
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
    print(f'{len(mutations)}/{len(mutations)} private figure mutations rejected; production source unchanged')


if __name__ == '__main__':
    main()
