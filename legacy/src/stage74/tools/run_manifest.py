"""Emit reviewable FUTURE-only configured runner inputs. Never authorizes runs."""
import argparse,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--config',required=True);p.add_argument('--build',required=True);p.add_argument('--output',required=True);a=p.parse_args()
 cfg=json.loads((ROOT/a.config).read_text());build=(ROOT/a.build).resolve();result=json.loads((build/'result.json').read_text());analysis=json.loads((build/'analysis.json').read_text());out=(ROOT/a.output).resolve()
 assert build.is_relative_to(ROOT/'stage74/builds')and out.is_relative_to(ROOT/'stage74')and not out.exists()
 if cfg['frozen_input_manifest']:
  m=json.loads((ROOT/cfg['frozen_input_manifest']).read_text())
  m.update(status='FUTURE_ONLY_NEW_ARTIFACT_NOT_DIFFERENTIALLY_TESTED',id=cfg['id'],runtime_eligible=True,
   execution_scope='New Stage74 configured SR rebuild '+cfg['id']+'; same fixed historical Stage73 continuation; requires a new separate explicit approval. Stage73 old attempts cannot be reused.',
   claim_directory='stage74/evidence',native_library=str(Path(result['artifact']).relative_to(ROOT)),
   return_guest_PCs=[int(q['guest_PC'],16)for q in result['return_bindings']],
   original_instructions=[dict(pc=q['pc'],size=q['end']-q['pc'])for q in analysis['instructions']])
  sources=[a.config,a.build+'/result.json',a.build+'/protected.ll',str(Path(result['artifact']).relative_to(ROOT))]
  sources +=[str(p.relative_to(ROOT))for p in (ROOT/'stage74/tools').glob('*.py')]
  m['pinned_inputs_and_sources'] +=[dict(path=p,sha256=sha(ROOT/p))for p in sources]
 else:
  m=dict(status='OPEN_CLOSURE_EXECUTION_REFUSED',id=cfg['id'],runtime_eligible=False,
   execution_scope='Phase8 business subset only; unresolved virtual calls, external boundary and missing exact input CPU',
   limits=dict(guest_seconds=8,hard_process_seconds=60),reasons=['3 original virtual-call targets unbound','External boundary_413a50 intentionally unresolved','No exact historical phase8 entry CPU snapshot supplied'])
 out.write_text(json.dumps(m,indent=2)+'\n')
 pending=dict(status='PENDING_USER_APPROVAL',user_message_id='',manifest_sha256=sha(out),scope=m['execution_scope'],maximum_attempts=dict(reference=1,native=1))
 out.with_suffix('.approval.example.json').write_text(json.dumps(pending,indent=2)+'\n')
 print(json.dumps(dict(manifest=str(out),runtime_eligible=m['runtime_eligible'],authorized=False)))
if __name__=='__main__':main()
