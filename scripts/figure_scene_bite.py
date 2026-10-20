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
        ('type11_callback', 'original_figure_image.hpp', 'f.action==4?0x4c14:0x4c13,archives', 'f.action==4?0x4c16:0x4c15,archives'),
        ('frame_reset', 'original_figure_update.hpp', 'if(int(out.counter)>=frames)out.counter=0;\n    if(speedArgument)', 'if(int(out.counter)>=frames)out.counter=2;\n    if(speedArgument)'),
        ('effect_position', 'original_figure_update.hpp', 'int(f.effectX)/8+int(f.x)*15', 'int(f.effectX)/8+int(f.x)*20'),
        ('scheduler', 'original_figure_update.hpp', ']+2)return out;', ']+1)return out;'),
        ('pheasant_idle', 'original_figure_update.hpp', 'if(idle>=200)f.action=30;', 'if(idle>200)f.action=30;'),
        ('pheasant_cooldown', 'original_figure_update.hpp', 'if(cooldown>0)--cooldown;', 'if(cooldown>=0)--cooldown;'),
        ('scheduler_load', 'original_figure_update.hpp', 'if(!networkActive)live.speed=personalSpeed;', 'if(networkActive)live.speed=personalSpeed;'),
        ('route_phase', 'original_figure_update.hpp', 'phase>(speed==4?1:2)?0:1;', 'phase>(speed==4?1:3)?0:1;'),
        ('route_step', 'original_figure_update.hpp', '++f.progress;', 'f.progress+=2;'),
        ('pool_order', 'original_figure_update.hpp', 'for(size_t slot=1;slot<2000;++slot)', 'for(size_t slot=1999;slot>0;--slot)'),
        ('rng_saved', 'original_figure.hpp', 'first=1299340,second=1299344;', 'first=1299344,second=1299340;'),
        ('rng_step', 'original_figure_update.hpp', 'value>>4)^value', 'value>>3)^value'),
        ('passability_water', 'original_figure_update.hpp', 'else if(t&0x40)mask=4;', 'else if(t&0x40)mask=0;'),
        ('reachability', 'original_figure_update.hpp', 'constexpr int offsets[]={-228,1,228,-1};', 'constexpr int offsets[]={-229,1,229,-1};'),
        ('spawn_offset', 'original_figure.hpp', 'at+853+4*n', 'at+853+2*n'),
        ('destination_rng', 'original_figure_update.hpp', 'x+=int(14*scale/4)', 'x+=int(14*scale/2)'),
        ('ordered_rng', 'original_figure_update.hpp', 'if(result.services)context->services=result.services;', 'if(result.services)context->services=before.services;'),
        ('crossing_prior', 'original_figure_update.hpp', 'next.priorX=f.x;next.priorY=f.y;', 'next.priorX=f.x+1;next.priorY=f.y;'),
        ('cart_delay', 'original_figure_update.hpp', 'saved.action==9?13:18;', 'saved.action==9?13:17;'),
        ('segment_cursor', 'original_figure_update.hpp', 'f.routeCursor=originalFigureWord(int(f.routeCursor)+1);', 'f.routeCursor=originalFigureWord(int(f.routeCursor)+2);'),
        ('worker_accepting', 'original_figure.hpp', 'link.accepting=data[at+7];', 'link.accepting=data[at+8];'),
        ('supplier_countdown', 'original_figure_update.hpp', 'base+unsigned(f.action==8)', 'base+unsigned(f.action==7)'),
        ('roam_accounting', 'original_figure_update.hpp', 'int(f.roamSteps)+int(speed)', 'int(f.roamSteps)+1'),
        ('free_arrival', 'original_figure_update.hpp', 'const bool arrived=int(f.freeRemainingX)+f.freeRemainingY<1;', 'const bool arrived=int(f.freeRemainingX)+f.freeRemainingY<=1;'),
        ('follower_speed', 'original_figure_update.hpp', 'case 13:return 2-unsigned(phase>2);', 'case 13:return 2+unsigned(phase>2);'),
        ('saved_fire_offset', 'original_figure.hpp', 'fireCount=fireBlocks80+2*80;', 'fireCount=fireBlocks80+2*80-4;'),
        ('inspector_fire_gate', 'original_figure_update.hpp', 'if(*context->services->fireCount>0)', 'if(*context->services->fireCount>=0)'),
        ('inspector_fire_threshold', 'original_figure_update.hpp', 'if(f.searchCounter>=20)', 'if(f.searchCounter>20)'),
        ('worker_employment', 'original_figure_update.hpp', 'int(house.residents)+f.employmentAmount', 'int(house.residents)+1'),
        ('entertainment_grid', 'original_figure_update.hpp', 'serviceGrid[f.cell]|=7u<<9;', 'serviceGrid[f.cell]|=7u<<6;'),
        ('population_frame', 'original_figure.hpp', 'sizeAnchor-28-152-bytes;', 'sizeAnchor-28-152-bytes+4;'),
        ('worker_removal', 'original_figure_update.hpp', 'reset.priorProgress=f.priorProgress;', 'reset.priorProgress=0;'),
        ('load_population', 'original_figure_update.hpp', 'services.population->total=int32_t(total);', 'services.population->total+=int32_t(total);'),
        ('zero_gate', 'original_building_scene.hpp', 'if(!f.image){', 'if(!f.image&&source==OriginalFigureRenderSource::Saved){'),
        ('effect_draw_cell', 'original_figure.hpp', '(f.type==1?229u:0u)', '(f.type==1?228u:0u)'),
        ('effect_extra_height', 'original_figure.hpp', 'x+y-int(f.raise)+f.extraY-int(f.height)', 'x+y-int(f.raise)+f.extraY'),
        ('animation_clock', 'original_building_animation.hpp', 'uint32_t(20*k)', 'uint32_t(19*k)'),
        ('timeline_repeat', 'original_scene_timeline.hpp', 'context=*out.lastPass.context;++out.updates;', 'context=initial;++out.updates;'),
        ('timeline_draw', 'original_scene_timeline.hpp', 'out.animationClock.flags);', 'std::array<bool,51>{});'),
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
                      'city19_empire2_layout.hpp', 'city19_empire_grid15_layout.hpp', 'city19_later7_layout.hpp']:
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
