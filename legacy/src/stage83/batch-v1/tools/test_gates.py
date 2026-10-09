"""Own data only; no Unicorn, EXE, SR function or original checkpoint executed."""
import copy,json,os,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
from common import ROOT,S,V,sha,put
import pipeline
from write_gate import check_path,sandbox_command
def main():
 c=json.loads((V/'configs/round01.json').read_text());cfg=V/'configs/round01.json';cases=[]
 def denied(name,fn):
  try:fn()
  except(Exception)as e:cases.append({'case':name,'denied':True,'reason':str(e)});return
  raise AssertionError('UNEXPECTED_PERMISSION '+name)
 for name,key,value in [('round11','round',11),('round0','round',0),('missing_approval','approval_sha256','0'*64),('changed_original_input','source','runs/fake/state.json'),('raised_step_budget','limits',{'capture':{'steps':6000001}}),('cross_round_output','approved_output_root',str(S/'runs/batch-v1-round02'))]:
  q=copy.deepcopy(c);q[key]=value;denied(name,lambda q=q:pipeline.validate(q,cfg))
 out=S/'runs/batch-v1-gate-tests';out.mkdir();outside=V/'evidence/owned-sentinel.txt';outside.write_text('PRESERVE\n');h=sha(outside)
 (out/'escape').symlink_to(V/'evidence',target_is_directory=True)
 for name,p in [('outside',outside),('symlink',out/'escape/owned-sentinel.txt'),('traversal',out/'../parent.txt'),('old_ledger',ROOT/'evidence/stage55-batch-ledger.json'),('other_round',S/'runs/batch-v1-round02/writes.bin')]:denied(name,lambda p=p:check_path(p,out))
 claim=out/'own.claim.json';put(claim,{'own_test':True});denied('duplicate_role_claim',lambda:put(claim,{'own_test':True}))
 f=pipeline.accounting;pipeline.accounting=lambda:{'rounds_used':10};denied('ten_rounds_consumed',lambda:pipeline.reserve(c,cfg,out/'unused'));pipeline.accounting=f;assert not(out/'unused').exists()
 cmd=['/usr/bin/clang','-std=c11','-O1','-Wall','-Wextra','-Werror',str(S/'tools/write_probe.c'),'-o',str(out/'probe')];q=subprocess.run(cmd,capture_output=True,timeout=30);assert q.returncode==0,q.stderr
 q=subprocess.run(sandbox_command(out,[out/'probe',out/'inside.txt',outside,out/'escape/owned-sentinel.txt',out/'../parent.txt']),capture_output=True,timeout=10);assert q.returncode==0,q.stderr;assert sha(outside)==h
 cases.extend({'case':n,'denied':True,'reason':'macOS kernel deny'}for n in ('C_outside','C_symlink','C_traversal'))
 code='import socket; s=socket.socket();\ntry:s.connect(("127.0.0.1",9))\nexcept PermissionError:print("network denied")\nelse:raise AssertionError("network allowed")'
 q=subprocess.run(sandbox_command(out,['/usr/bin/python3','-B','-c',code]),capture_output=True,timeout=10);assert q.returncode==0 and b'network denied'in q.stdout,(q.stdout,q.stderr);cases.append({'case':'kernel_network','denied':True})
 native=S/'builds/batch-v1-authored/native-driver';q=subprocess.run([str(native),'NO_INPUT',str(out),'NO_LIMITS','NO_TICKET'],capture_output=True,timeout=10);assert q.returncode==3 and b'NATIVE_WRITE_GATE_OUTPUT_REFUSED'in q.stderr;cases.append({'case':'native_wrong_output_before_ticket_or_game','denied':True})
 assert pipeline.accounting()['rounds_used']==0
 put(V/'evidence/gate-tests.json',{'status':'COUNTER_WRITE_AND_NATIVE_GATE_OWN_TESTS_PASSED','negative_cases':cases,'negative_count':len(cases),'outside_sentinel_unchanged':True,'original_game_or_Unicorn_execution':0,'old_ledger_write':0});print(json.dumps({'negative_cases_passed':len(cases),'original_game_execution':0}))
if __name__=='__main__':main()
