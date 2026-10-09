"""Consume exact measured CPU evidence, never re-execute the diagnostic."""
import hashlib,json,struct,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage80'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 source=ROOT/'stage79/runs/cpu-evidence-once/result.json';result=json.loads(source.read_text());assert result['status']=='PUBLIC_CPU_AND_SCOPED_CACHE_PROBES_COLLECTED'and result['original_game_instructions']==0 and len(result['probes'])==56
 meta=json.loads((ROOT/'stage79/evidence/checkpoint-state.json').read_text());contexts={r['tid']:r for r in result['contexts']};assert set(contexts)=={1,2,3,4};copy=json.loads(json.dumps(meta));cert=[]
 for tid,c in sorted(contexts.items()):
  row=copy['thread_records'][str(tid)];p=c['public_CPU'];assert p['FS_BASE']==row['teb']and p['CR0']&1 and not p['CR0']&0x80000000 and not p['EFLAGS']&((1<<17)|(1<<8));assert(p['CS'],p['DS'],p['ES'],p['SS'])==(8,16,16,16)
  probes=[q for q in result['probes']if q['tid']==tid];assert len(probes)==14
  expected={(seg,addr,kind)for seg in ['DS','ES','SS']for addr in [0,0xfffffffc]for kind in ['read','write']}|{('FS',0,kind)for kind in ['read','write']};assert {(q['segment'],q['target'],q['access'])for q in probes}==expected
  assert all(q['expected_boundary_observed']and q['authored_instructions_maximum']==1 for q in probes)
  row['public_CPU']=p;row['cached_segments_verified']=True;row['cached_segment_scope']={'DS_ES_SS':'measured 32bit dword read/write endpoint linear addresses','FS':'offset0 dword read/write only','GS_BASE':'not publicly readable in UC_MODE_32; captured zero is not proof'}
  cert.append(dict(tid=tid,FS_base=p['FS_BASE'],probe_records=14,FS_offsets_allowed=[0],cached_FS_limit_value_known=False,GS_BASE_supported=False,source_result_sha256=sha(source)))
 assert all(contexts[1]['public_CPU'][k]==v for k,v in meta['root_current_CPU'].items());copy['root_current_CPU']=contexts[1]['public_CPU'];copy['public_worker_CPU_missing']=False;copy['CPU_evidence_source']=str(source);copy['CPU_evidence_sha256']=sha(source);copy['engine_GS_BASE_query_is_unknown']=True
 (S/'evidence/measured-state.json').write_text(json.dumps(copy,indent=2)+'\n');(S/'evidence/measured-cache-certificates.json').write_text(json.dumps(dict(certificates=cert,scope_is_not_general_segmentation_or_SEH_proof=True),indent=2)+'\n')
 core=['EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI','EIP','EFLAGS'];selectors=['CS','DS','SS','ES','FS','GS'];body=bytearray(b'S80T'+struct.pack('<III',1,4,meta['root_host']['current_tid']))
 for tid,c in sorted(contexts.items()):
  row=copy['thread_records'][str(tid)];p=c['public_CPU'];current=tid==meta['root_host']['current_tid'];tls=meta['root_host']['tls']if current else row['tls'];error=meta['root_host']['last_error']if current else row['last_error'];allocated=sum(1<<int(i)for i in tls);values=[tls.get(str(i),0)for i in range(64)];status={'runnable':1,'waiting':2,'exited':3}[row['status']]
  opaque=Path(meta['root_opaque_context']['path']).read_bytes()if current else(ROOT/row['opaque_context']['path']).read_bytes();assert hashlib.sha256(opaque).hexdigest()==c['opaque_sha256']
  body+=struct.pack('<4IQ',tid,status,error,row['teb'],allocated)+struct.pack('<64I',*values)+struct.pack('<10I',*(p[k]for k in core))+struct.pack('<6I',*(p[k]for k in selectors))+struct.pack('<I',p['CR0'])
  for name in ['GDTR','LDTR']:body+=struct.pack('<4Q',*p[name])
  body+=struct.pack('<2Q3I',p['FS_BASE'],p['GS_BASE'],p['FPCW'],p['FPSW'],p['FPTAG'])
  for pair in p['FP80']:body+=struct.pack('<QH',*pair)
  body+=opaque
 (S/'evidence/measured-thread-transfer.bin').write_bytes(body);print(json.dumps(dict(status='FOUR_MEASURED_PUBLIC_CPU_AND_SCOPED_CACHE_RECORDS_IMPORTED',root_core_matches_frozen=True,worker_public_CPU_available=True,GS_BASE_query_supported=False,opaque_offsets_guessed=False,original_game_instructions=0,transfer_bytes=len(body))))
if __name__=='__main__':main()
