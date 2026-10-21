"""Building update pass tests; real inputs read only EMPEROR_GAMEDATA_ROOT."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TEMP = Path('/private/var/folders/5n/nsp1d6sn0_76p_0f_j_dmj_c0000gn/T/opencode')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--bite', action='store_true')
    parser.add_argument('--bite-kind', choices=['crop', 'industry', 'road', 'accounting', 'religion', 'migration', 'emigration', 'overflow', 'population', 'employment', 'market', 'stall', 'load', 'resource', 'hook', 'hunter'], default='crop')
    args = parser.parse_args()
    if args.build_dir and not args.build_dir.is_absolute():
        parser.error('absolute build directory required')
    build = (args.build_dir or Path(tempfile.mkdtemp(prefix='b-building-update-', dir=TEMP))).resolve()
    configured = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if configured and (build == Path(configured).resolve() or Path(configured).resolve() in build.parents):
        parser.error('build directory must stay outside GameData')
    build.mkdir(parents=True, exist_ok=True)
    sources = ['original_building_update_tests.cpp', 'original_map.cpp', 'original_save.cpp',
               'original_pak.cpp', 'original_terrain.cpp', 'original_ground.cpp', 'map_reference.cpp', 'resource_root.cpp', 'sg3.cpp']
    flags = ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    test_source = ROOT / 'game_lab' / sources[0]
    if args.bite:
        # Mutate only private authored source. Original bytes/listings never copied.
        copied = build / 'bite'
        copied.mkdir(exist_ok=True)
        header = (ROOT / 'game_lab/original_building_update.hpp').read_text()
        mutations = {
            'crop': ('low>32?', 'low>=32?', 'literal per-type growth uses LOW byte32 threshold'),
            'industry': ('?5:10)', '?5:9)', 'industry workforce100 percent produces10 with ordinary divisor10'),
            'road': ('if(r<bestRank)', 'if(r<=bestRank)', 'equal network rank preserves clockwise first candidate'),
            'accounting': ('if(r.priority>5)r.priority=3', 'if(r.priority>5)r.priority=4', 'out-of-range positive priority normalizes to3 before demand test'),
            'religion': ('activeDelta[]={-4,-5,-6,-7,-8}', 'activeDelta[]={-4,-5,-5,-7,-8}', 'religion difficulty decrement precedes aggregate and disabled status reset'),
            'migration': ('const int amount=std::min(left,pass<2?6:int(h.free))', 'const int amount=std::min(left,pass<2?5:int(h.free))', 'immigrant constructor and caller writes preserve independent literal fields'),
            'emigration': ('c.labor->lastPopulationDelta=originalBuildingSub32(0,count)', 'c.labor->lastPopulationDelta=count', 'emigration stores signed last arrival and population before allocating type12'),
            'overflow': ('originalBuildingSub32(c.labor->excessDepartureTotal,int(i.free))', 'originalBuildingAdd32(c.labor->excessDepartureTotal,int(i.free))', 'overflow departure increments separate accounting total only after type13 allocation'),
            'population': ('l.health>=61?2:1', 'l.health>=61?3:1', 'pending population statistics grows ordered ordinary house before sample average'),
            'employment': ('const int amount=std::min(candidate.required-int(i.workers),left)', 'const int amount=std::min(candidate.required-int(i.workers),0)', 'employment distributes ordered integer remainder after proportional first pass'),
            'market': ('(rotation?3u:684u)', '(rotation?3u:685u)', 'Market orientation0 uses descriptor entrance at origin plus684'),
            'stall': ('if(out)out->network.reset()', 'if(out)out->network=0', 'Stall copies registered Market road fields and does not dispatch network setter'),
            'load': ('c.loadAccountingApplied=true;', 'c.loadAccountingApplied=false;', 'explicit UI load accounting transition is applied once before scheduled phase1'),
            'resource': ('value=next<old?255:next', 'value=next<old?254:next', 'resource unsigned byte overflow saturates at255 and grows owned cells'),
            'hook': ('if(i.spawnCounter>limit)', 'if(i.spawnCounter>=limit)', 'Well exact counter boundary advances without premature allocation'),
            'hunter': ('if(i.spawnCounter<=threshold)', 'if(i.spawnCounter<threshold)', 'Hunter hook counter1 closes before its strict greater-than1 allocation branch'),
        }
        before, after, assertion = mutations[args.bite_kind]
        assert header.count(before) == 1
        (copied / 'original_building_update.hpp').write_text(header.replace(before, after, 1))
        test_source = copied / sources[0]
        test_source.write_text((ROOT / 'game_lab' / sources[0]).read_text())
    binary = build / ('building-update-' + args.mode + ('-bite' if args.bite else ''))
    subprocess.run(['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror', *flags,
                    '-I', str(ROOT / 'game_lab'), str(test_source),
                     *[str(ROOT / 'game_lab' / s) for s in sources[1:]], '-framework', 'CoreGraphics', '-framework', 'CoreFoundation', '-lz', '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], check=False, capture_output=args.bite, text=args.bite)
    if args.bite:
        if result.returncode == 0:
            raise RuntimeError('bite mutation survived')
        if assertion not in result.stderr:
            raise RuntimeError('bite failed outside the intended assertion: ' + result.stderr)
        print(result.stderr.strip())
        print('{"bite_detected":true,"mutation":"' + args.bite_kind + '"}')
        return 0
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
