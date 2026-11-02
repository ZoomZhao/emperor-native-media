"""App stored-surface adapter checks; metrics only read process-local data root."""
import os
import sys
if len(sys.argv)>1 and sys.argv[1] in ('metrics','constraints','playback','primary-metric','refusal-census') and not os.environ.get('EMPEROR_GAMEDATA_ROOT'):
    print('{"skipped":true,"reason":"EMPEROR_GAMEDATA_ROOT unset or empty"}')
    raise SystemExit(0)
from pathlib import Path
import argparse
import subprocess
import tempfile
import hashlib
import json
import fcntl
import shlex
from concurrent.futures import ThreadPoolExecutor

ROOT=Path(__file__).resolve().parents[1]
def build_binary(build,mode,sources,flags):
    binary=build/('app-map-surface-'+mode)
    objects_dir=build/('objects-'+mode);objects_dir.mkdir(parents=True,exist_ok=True)
    signature=json.dumps([str(ROOT),sources,['-std=c++17','-Wall','-Wextra','-Werror','-MD',*flags],
                          [os.environ.get(k) for k in ('SDKROOT','DEVELOPER_DIR','CPATH','CPLUS_INCLUDE_PATH','MACOSX_DEPLOYMENT_TARGET')],
                          os.stat('/usr/bin/clang++').st_mtime_ns])
    with (build/'.compile-lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        signature_file=objects_dir/'flags.json'
        same_flags=signature_file.exists() and signature_file.read_text()==signature
        objects=[];changed=False
        for source in sources:
            obj=objects_dir/(source.replace('/','-')+'.o');dep=obj.with_suffix('.d')
            fresh=same_flags and obj.exists() and dep.exists()
            if fresh:
                try:
                    dependencies=shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1])
                    fresh=bool(dependencies) and all(Path(p).exists() and Path(p).stat().st_mtime_ns<=obj.stat().st_mtime_ns for p in dependencies)
                except (OSError,ValueError,IndexError):fresh=False
            if not fresh:
                temporary=obj.with_suffix('.tmp.o');temporary_dep=dep.with_suffix('.tmp.d')
                subprocess.run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-MD','-MF',str(temporary_dep),'-MT','cache-target','-c',str(ROOT/source),'-o',str(temporary)],check=True)
                os.replace(temporary,obj);os.replace(temporary_dep,dep);changed=True
            objects.append(obj)
        if changed or not binary.exists() or any(obj.stat().st_mtime_ns>binary.stat().st_mtime_ns for obj in objects):
            temporary=binary.with_suffix('.tmp')
            subprocess.run(['/usr/bin/clang++',*flags,*map(str,objects),'-framework','CoreGraphics','-framework','CoreFoundation','-lz','-o',str(temporary)],check=True)
            os.replace(temporary,binary)
        signature_file.write_text(signature)
    return binary

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
    p.add_argument('task',choices=['test','metrics','constraints','playback','primary-metric','refusal-census']);p.add_argument('--mode',choices=['release','undefined'],default='release');p.add_argument('--build-dir',type=Path);p.add_argument('--updates',type=int,default=100)
    a=p.parse_args();cache='emperor-app-surface-'+hashlib.sha256(str(ROOT).encode()).hexdigest()[:12]
    build=(a.build_dir or Path(tempfile.gettempdir())/cache).resolve()
    configured=os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if configured and build.is_relative_to(Path(configured).resolve()):p.error('--build-dir must be outside the data root')
    build.mkdir(parents=True,exist_ok=True)
    sources=['game_lab/app/map_surface.cpp','game_lab/app/map_surface_preview.cpp','game_lab/app/city_playback.cpp','game_lab/app/map_surface_tests.cpp','game_lab/map_reference.cpp','game_lab/original_map.cpp','game_lab/original_terrain.cpp','game_lab/original_ground.cpp','game_lab/original_save.cpp','game_lab/original_pak.cpp','game_lab/sg3.cpp','game_lab/resource_root.cpp']
    flags=['-O2']if a.mode=='release'else['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']
    binary=build_binary(build,a.mode,sources,flags)
    fixture=Path(tempfile.mkdtemp(prefix='owned-fixtures-',dir=build))if a.task=='test'else None
    if a.task in ('playback','primary-metric','refusal-census'):
        argv=[str(binary),'--'+a.task]
        if a.task in ('primary-metric','refusal-census'):
            if not 1<=a.updates<=1000000:p.error('--updates needs 1..1000000')
            argv+=['--updates',str(a.updates)]
        if a.task=='primary-metric':
            def one(index):
                return subprocess.run([str(binary),'--primary-one','--updates',str(a.updates),'--index',str(index)],check=False,capture_output=True,text=True)
            with ThreadPoolExecutor(max_workers=6) as workers:
                results=list(workers.map(one,range(6)))
            qualified=0;names=set()
            for result in results:
                if result.returncode:
                    sys.stdout.write(result.stdout);sys.stderr.write(result.stderr);return result.returncode
                rows=result.stdout.splitlines()
                if len(rows)!=1:raise RuntimeError('primary source did not produce one row')
                columns=rows[0].split('\t')
                if len(columns)!=4 or columns[0] in names:raise RuntimeError('invalid or duplicate primary source row')
                names.add(columns[0]);qualified+=int(columns[1])>=a.updates and columns[3]=='none'
                print(rows[0])
            print('PRIMARY %d/6'%qualified);return 0
        result=subprocess.run(argv,check=False,capture_output=True,text=True)
        sys.stdout.write(result.stdout);sys.stderr.write(result.stderr)
        return result.returncode
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
