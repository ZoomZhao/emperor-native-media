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
    parser.add_argument('--bite-kind', choices=['crop', 'industry', 'road', 'accounting', 'religion', 'migration', 'emigration', 'overflow', 'population', 'employment', 'market', 'stall', 'load', 'resource', 'hook', 'hunter', 'trade', 'totals', 'aggregate', 'model', 'rectangle', 'service', 'farmer', 'crop-order', 'bay', 'evolution', 'lifecycle', 'fire', 'risk', 'effect', 'counter', 'tail', 'timer', 'timer-faction', 'saved-tail', 'saved-crop-festival', 'stall-capacity', 'storage-export', 'station', 'house-model'], default='crop')
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
            'trade': ('if(p.timer>0){--p.timer;', 'if(p.timer>0){++p.timer;', 'active saved trade quota decrements dispatch timer after resetting and saturating four counters'),
            'totals': ('f.delivery!=int(e.first)', 'f.home!=int(e.first)', 'totals preserves high resource rows and coverage and validates figure delivery link rather than home'),
            'aggregate': ('h.at(55-4)=uint8_t(', 'h.at(54-4)=uint8_t(', 'House1 aggregation writes literal service55 religion54 and preserves entertainment53'),
            'model': ('originalBuildingPercent(row[k],modifier[k])', 'originalBuildingPercent(row[k],100)', 'building model difficulty multiplies thirteen independent columns with signed truncation'),
            'rectangle': ('if(++counter>=m[2])', 'if(counter++>=m[2])', 'Market rectangle uses inclusive descriptor bounds and initial counter1 delta schedule'),
            'service': ('f.action=6;f.counter=30;', 'f.action=6;f.counter=29;', 'Well positive allocation publishes reciprocal identity action and delay after strict counter gate'),
            'farmer': ('if(skip){--skip;target.primaryFigure=0;', 'if(skip){--skip;target.primaryFigure=19;', 'farmer grouping clears skipped candidates rather than assigning the new farmer'),
            'crop-order': ('return a.first<b.first;', 'return a.first!=b.first?a.first<b.first:a.second>b.second;', 'farmer grouping clears skipped candidates rather than assigning the new farmer'),
            'bay': ('if(bay.resource&&bay.amount>=100)', 'if(bay.resource&&bay.amount>100)', 'fifth crop bay at100 reaches named positive cart boundary'),
            'evolution': ('direction==-1&&delay<9', 'direction==-1&&delay<8', 'house failed requirement waits through old delay8 before conversion'),
            'lifecycle': ('int(*i.modelDesirability)>=threshold&&!upgraded', 'int(*i.modelDesirability)>threshold&&!upgraded', 'phase40 inclusive desirability threshold upgrades model and existing Well selector'),
            'fire': ('if(i.burning<0)i.burning=0;', 'if(i.burning<0)i.burning=1;', 'fire pass normalizes negative burning before zero-fire exit'),
            'risk': ('uint16_t(i.damageRisk)+3', 'uint16_t(i.damageRisk)+2', 'risk phase0 and region0 add model fire3 and damage2 plus fixed3'),
            'effect': ('((value-1)<<bit)', '((value)<<bit)', 'eighth monument pass decays packed3-bit lanes and preserves upper five bits'),
            'counter': ('c.phase47Counters.fill(0)', 'c.phase47Counters.fill(1)', 'phase47 clears four literal counters and advances to48'),
            'tail': ('++*c.cycle;++*c.monthPart;', '++*c.cycle;', 'ordinary phase50 increments both clocks smooths health59 to60 and resets phase'),
            'timer': ('child[1][n],1),0,48', 'child[1][n],1),0,49', 'part2 current faction21 decrements last timer and releases reserves before capacity increment'),
            'timer-faction': ('partner.relationTimers[size_t(faction)]', 'partner.relationTimers[21]', 'part2 current faction0 selects first child and preserves last child'),
            'saved-tail': ('currentHealthRating=value(12099)', 'currentHealthRating=value(11392)', 'cycle tail saved health rating uses final59501B wire12099 rather than11392 decoy'),
            'saved-crop-festival': ('detail::u32(data.data()+festival+71)==4;', 'detail::u32(data.data()+festival+71)==2;', 'crop festival restores saved enabled mode3 deity4 rather than production deity2'),
            'stall-capacity': ('resource==28?400u:200u', 'resource==28?399u:200u', 'Stall saved bay399 reaches supplier boundary below resource28 capacity400'),
            'storage-export': ('if(percent<50)return -1;', 'if(percent<51)return -1;', 'storage export accepts earliest mode3 bay only from staffing50'),
            'station': ('if(type==58)i.available=1;', 'if(type==58)i.suppression=1;', 'TradingStation hook writes runtime48 availability while preserving legacy suppression contract'),
            'house-model': ('originalBuildingAdd32(row[k],modifier[k])', 'originalBuildingSub32(row[k],modifier[k])', 'HOUSE MODS adds selected row then clamps all24 independent model columns'),
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
