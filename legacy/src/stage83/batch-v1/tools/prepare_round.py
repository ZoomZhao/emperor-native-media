"""Prepare/build/freeze one fresh round; never execute guest code."""
import argparse,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
from common import ROOT,S,V,sha,put
def main():
 p=argparse.ArgumentParser();p.add_argument('--round',type=int,required=True);a=p.parse_args();assert 1<=a.round<=10
 label='round%02d'%a.round;cfg=V/'configs'/(label+'.json');pre=V/'configs'/(label+'.build.json');out=S/'builds'/('batch-v1-'+label)
 if cfg.exists()or pre.exists()or out.exists()or(S/'runs'/('batch-v1-'+label)).exists():raise RuntimeError('FRESH_ROUND_AND_BUILD_REQUIRED')
 c=json.loads((S/'configs/original.json').read_text());c.update(id='S83_batch_v1_'+label,round=a.round,approval_sha256=sha(V/'evidence/authorization.json'),approved_output_root=str(S/'runs'/('batch-v1-'+label)),native_driver=str((out/'native-driver').relative_to(ROOT)),host_write_policy='stage83/batch-v1/configs/round-write-policy.sb')
 c['pins'].update({str(f.relative_to(ROOT)):sha(f)for f in(V/'tools').iterdir()if f.is_file()});c['pins']['stage83/batch-v1/configs/round-write-policy.sb']=sha(V/'configs/round-write-policy.sb');put(pre,c)
 q=subprocess.run(['/usr/bin/python3','-B',str(V/'tools/build_drivers.py'),'--config',str(pre),'--out',str(out)],cwd=ROOT,capture_output=True,timeout=60);put(V/'evidence'/(label+'-prepare.json'),{'command_kind':'build-only','returncode':q.returncode,'stdout':q.stdout.decode(),'stderr':q.stderr.decode(),'original_game_execution':0})
 if q.returncode:raise RuntimeError('BUILD_FAILED_FROZEN_LOGS')
 c['pins'].update({str(f.relative_to(ROOT)):sha(f)for f in out.iterdir()if f.is_file()});put(cfg,c);print(json.dumps({'frozen_config':str(cfg),'game_execution':0,'note':'Current unchanged input has a proven capture-budget blocker; preparing does not resolve it.'}))
if __name__=='__main__':main()
