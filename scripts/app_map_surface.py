"""App stored-surface adapter checks; metrics only read process-local data root."""
import os
import sys
if len(sys.argv)>1 and sys.argv[1] in ('metrics','constraints') and not os.environ.get('EMPEROR_GAMEDATA_ROOT'):
    print('{"skipped":true,"reason":"EMPEROR_GAMEDATA_ROOT unset or empty"}')
    raise SystemExit(0)
from pathlib import Path
import argparse
import subprocess
import tempfile
import hashlib
import json

ROOT=Path(__file__).resolve().parents[1]
def constraint_summary(metrics):
    rows=metrics['residual_city_constraints']
    discovery=[r for r in rows if hashlib.sha256(r['city'].encode('utf-8')).digest()[0]%2==0]
    families={r['city'].removesuffix('_S') for r in discovery}
    held=[r for r in rows if r not in discovery]
    overlap=[r for r in held if r['city'].removesuffix('_S') in families]
    strict=[r for r in held if r not in overlap]
    # Filename-family sensitivity,not a claim of independent historical cities.
    broad=lambda name:name.removesuffix('_S').replace('Chang-an Han','Chang-an').replace('Chang-an Sui','Chang-an')
    broad_known={broad(r['city']) for r in discovery}
    broad_strict=[r for r in held if broad(r['city']) not in broad_known]
    def summarize(group):
        fields=['cells','side_conflict_cells','complete_group_cells','complete_groups','mirrored_cells','crop_touching_cells','marker_constraint_cells']
        return {'cities':[r['city'] for r in group],
                'city_families':len({r['city'].removesuffix('_S') for r in group}),
                **{key:sum(r[key] for r in group) for key in fields}}
    return {'passed':True,'source_cities':metrics['App_surface_sources'],
            'rule':'subTile low nibble +1 square;draw low-six offsets;one implied anchor;exact same-image membership',
            'split':'SHA256 city-stem UTF-8 first-byte parity;even discovery;odd holdout;exclude discovery _S families from strict holdout',
            'discovery':summarize(discovery),'initial_holdout':summarize(held),
            'family_overlap_excluded':summarize(overlap),'strict_holdout':summarize(strict),
            'family_definition':'strip terminal _S only;broader shared provenance is unproved',
            'Chang_an_merged_sensitivity':summarize(broad_strict),
            'all_residual_cells':metrics['residue'],
            'all_complete_group_cells':metrics['residual_complete_group_cells'],
            'all_complete_groups':metrics['residual_complete_groups'],
            'storage_group_rule_passed':metrics['residual_complete_group_cells']==metrics['residue'],
            'residual_marker_constraint_cells':metrics['residual_marker_constraint_cells'],
            'complete_city_marker_controls':metrics['complete_city_marker_controls'],
            'marker_control_rule_passed':metrics['complete_city_marker_controls']['complete_group_cells']==metrics['complete_city_marker_controls']['marker_constraint_cells'],
            'rendering_coverage_increased':False,'original_painter_anchor_verified':False,
            'note':'Metadata subclasses first encountered in holdout do not have separate discovery support;marker rule was exploratory after aggregate holdout counts.'}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('task',choices=['test','metrics','constraints']);p.add_argument('--mode',choices=['release','undefined'],default='release');p.add_argument('--build-dir',type=Path)
    a=p.parse_args();build=(a.build_dir or Path(tempfile.mkdtemp(prefix='emperor-app-surface-'))).resolve()
    configured=os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if configured and build.is_relative_to(Path(configured).resolve()):p.error('--build-dir must be outside the data root')
    build.mkdir(parents=True,exist_ok=True)
    binary=build/('app-map-surface-'+a.mode)
    sources=['game_lab/app/map_surface.cpp','game_lab/app/map_surface_tests.cpp','game_lab/map_reference.cpp','game_lab/original_map.cpp','game_lab/original_terrain.cpp','game_lab/original_ground.cpp','game_lab/original_save.cpp','game_lab/sg3.cpp','game_lab/resource_root.cpp']
    flags=['-O2']if a.mode=='release'else['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']
    subprocess.run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,*[str(ROOT/s)for s in sources],'-framework','CoreGraphics','-framework','CoreFoundation','-lz','-o',str(binary)],check=True)
    fixture=Path(tempfile.mkdtemp(prefix='owned-fixtures-',dir=build))if a.task=='test'else None
    if a.task=='constraints':
        result=subprocess.run([str(binary),'--metrics'],check=False,capture_output=True,text=True)
        if result.returncode:
            sys.stderr.write(result.stderr);sys.stdout.write(result.stdout);return result.returncode
        metrics=json.loads(result.stdout)
        print(json.dumps(metrics if metrics.get('skipped') else constraint_summary(metrics),sort_keys=True))
        return 0
    if a.task=='test':
        result=subprocess.run([str(binary),str(fixture)],check=True,capture_output=True,text=True)
        report=json.loads(result.stdout)
        from app_surface_audit_cases import run
        report['residual_black_box_cases']=run(binary,build)
        print(json.dumps(report,sort_keys=True));return 0
    return subprocess.run([str(binary),'--metrics'if a.task=='metrics'else str(fixture)],check=False).returncode
if __name__=='__main__':raise SystemExit(main())
