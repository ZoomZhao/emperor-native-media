"""Bounded native review. Source read only; authored fixtures/builds in --out-dir."""
import argparse,hashlib,json,os,subprocess,tempfile
from pathlib import Path
from headless_persistence import extract,generate

HERE=Path(__file__).resolve().parent
def finish(report,out):
    positive=[r for r in report['records']if r['name']in ['properties','persistence']]
    negative=[r for r in report['records']if r['name']=='negative-slot']
    report['contracts_passed']=all(r['compile']['exit']==0 and r.get('run',{}).get('exit')==0 for r in positive)
    report['negative_slot_regression_passed']=all(r['compile']['exit']==0 and r.get('run',{}).get('exit')==0 for r in negative)
    memory=next(r for r in report['records']if r['name']=='metadata-memory')
    report['memory_budget_passed']=False
    if memory['compile']['exit']==0 and len(memory['records'])==2 and all(r['exit']==3 for r in memory['records']):
        rss=[json.loads(r['stdout'].splitlines()[-1])['peak_rss'] for r in memory['records']]
        report['metadata_peak_rss_delta']=rss[1]-rss[0]
        # Loose 8 MiB envelope for a 16 KiB parser budget; no exact RSS claim.
        report['memory_budget_passed']=report['metadata_peak_rss_delta']<=8*1024*1024
    report['passed']=all(report[k]for k in ['contracts_passed','negative_slot_regression_passed','memory_budget_passed'])
    (out/'review-results.json').write_text(json.dumps(report,indent=2))
    return 0 if report['passed']else 1

def main():
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument('--source-root',type=Path,required=True)
    a.add_argument('--out-dir',type=Path)
    a.add_argument('--mode',choices=['release','undefined','both'],default='both')
    x=a.parse_args();root=x.source_root.resolve();out=x.out_dir.resolve()if x.out_dir else Path(tempfile.mkdtemp(prefix='emperor-candidate-review-'))
    if out==root or root in out.parents:raise ValueError('review output must be outside source root')
    out.mkdir(parents=True,exist_ok=True);generate(root,out)
    source=(root/'game_lab/app/main.mm').read_text()
    mre=(HERE/'negative_slot_mre.mm').read_text();start=mre.index('@implementation D');end=mre.index('\n@end',start)
    methods='\n'.join(extract(source,n)for n in ['onOriginalBuildingLocate:','onOriginalManualAdopt:'])
    mre=mre[:start]+'@implementation D\n- (void)onOriginalMapPlaneChanged:(id)sender{(void)sender;}\n'+methods+mre[end:]
    (out/'negative_slot_current.mm').write_text(mre)
    env=os.environ.copy();env.pop('EMPEROR_GAMEDATA_ROOT',None)
    def run(cmd,timeout=60,extra=None):
        e=env.copy();e.update(extra or {})
        r=subprocess.run([str(v)for v in cmd],env=e,capture_output=True,text=True,timeout=timeout)
        return {'command':[str(v)for v in cmd],'exit':r.returncode,'stdout':r.stdout,'stderr':r.stderr}
    common=['/usr/bin/clang++','-std=c++17','-fobjc-arc','-I'+str(root)]
    report={'source_root':str(root),'out_dir':str(out),'main_sha256':hashlib.sha256(source.encode()).hexdigest(),'GUI_operations':0,'original_data':False,'records':[]}
    modes=['release','undefined']if x.mode=='both'else[x.mode]
    for mode in modes:
        flags=['-O2']if mode=='release'else['-O1','-g','-fsanitize=undefined,bounds','-fno-sanitize-recover=all','-D_LIBCPP_ENABLE_ASSERTIONS=1']
        for name,src in [('properties',HERE/'candidate_properties.mm'),('persistence',out/'persistence_callbacks.mm'),('negative-slot',out/'negative_slot_current.mm')]:
            binary=out/(name+'-'+mode);c=run(common+flags+[src,root/'game_lab/original_map.cpp','-framework','Foundation','-lz','-o',binary])
            record={'name':name,'mode':mode,'compile':c}
            if c['exit']==0:
                args=[binary]
                if name=='persistence':
                    fixture=out/('persistence-'+mode+'-fixtures');fixture.mkdir(exist_ok=True);args.append(fixture)
                record['run']=run(args,30)
            report['records'].append(record)
    binary=out/'metadata-memory-probe'
    c=run(common+['-O2',HERE/'metadata_memory_probe.mm']+[root/('game_lab/'+v+'.cpp')for v in ['original_map','original_pak','original_save','original_terrain','resource_root','sg3']]+['-framework','Foundation','-framework','CoreGraphics','-lz','-o',binary])
    mem={'name':'metadata-memory','compile':c,'records':[]}
    if c['exit']==0:
        data=out/'authored-data';data.mkdir(exist_ok=True)
        for size in [16385,32*1024*1024]:
            path=out/('metadata-'+str(size)+'.json')
            with path.open('wb')as f:f.truncate(size)
            r=run([binary,path],30,{'EMPEROR_GAMEDATA_ROOT':str(data)});r['file_bytes']=size;mem['records'].append(r)
    report['records'].append(mem)
    code=finish(report,out)
    for r in report['records']:
        print(r['name'],r.get('mode',''),r.get('run',{}).get('stdout','').strip(),r.get('run',{}).get('stderr','').strip())
    print(json.dumps({k:report[k]for k in ['contracts_passed','negative_slot_regression_passed','memory_budget_passed','metadata_peak_rss_delta','passed'] if k in report}))
    print('Report:',out/'review-results.json')
    return code
if __name__=='__main__':raise SystemExit(main())
