"""Self-authored instruction bytes and native helper state tests, no game calls."""
import ctypes,json,subprocess
from pathlib import Path
from stage70_analysis import decode,Analysis,PE,LOGIC,ALL
from build_pe import write_pe
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence';R=ROOT/'runs/stage70-self';R.mkdir(exist_ok=True)
class ByteInput:
    def __init__(self,b):self.b=bytes.fromhex(b)
    def read(self,p,n):
        q=self.b[p:p+n]
        if len(q)!=n:raise ValueError('truncated input')
        return q
def main():
    rows=[]
    cases=[('f3ab','rep_stos',32),('66f3ab','rep_stos',16),('f3aa','rep_stos',8),('668b08','mov',16),
        ('0fbf08','movsx',16),('0fbe08','movsx',8),('f7c178563412','test',32),('66f7c13412','test',16),('f6c504','test',8),('66b83412','mov',16),('fc','cld',None),('fd','std',None)]
    for raw,name,bits in cases:
        q=decode(ByteInput(raw),0);assert q['end']==len(bytes.fromhex(raw)) and q['name']==name
        if name=='rep_stos':assert q['args'][0]['bits']==bits and q['defines']==q['undefined']==0 and q['effects']['input_architectural_flags']==0x400
        elif name=='movsx':assert q['args'][1]['bits']==bits and q['args'][0]['bits']==32 and q['defines']==0
        elif bits:assert q['args'][0]['bits']==bits
        if raw=='f6c504':assert q['args'][0]==dict(kind='reg',reg=1,bits=8,shift=8)
        if name=='test':assert q['defines']==LOGIC and q['undefined']==4
        rows.append(dict(raw=raw,passed=True,instruction=q))
    for raw in ['f366f3ab','f2ab','67f3ab','f38b00','0fbf','668b','66c3']:
        try:decode(ByteInput(raw),0)
        except (ValueError,IndexError):rows.append(dict(raw=raw,rejected=True))
        else:raise AssertionError('unsupported prefix/truncation accepted '+raw)
    code=bytes.fromhex('83f801f3ab6690c3') # remove unsupported66NOP; marker fixture below replaces it.
    code=bytes.fromhex('83f801f3ab740233c0c3')
    path=R/'self-rep-preserves-comparison.exe';write_pe(path,[dict(name='.text',rva=4096,data=code,flags=0x60000020),dict(name='.data',rva=8192,data=bytes(32),flags=0xc0000040)],4096,[],{})
    a=Analysis(PE(str(path)),dict(roots=[0x401000],windows=[[0x401000,0x401000+len(code)]])).run();assert not a['unknowns'] and a['conversion_ready']
    proof=a['flags']['conditional_read_evidence'];assert any(q['consumer']=='0x401005' and q['producers']==['0x401000']for q in proof)
    (E/'stage70-self-byte-tests.json').write_text(json.dumps(dict(rows=rows,REP_preserves_prior_compare_flags=proof,passed=True),indent=2)+'\n')
    # Build and run only self-authored generic helper tests: no derived game code is linked.
    cmd=['/usr/bin/clang','-std=c11','-O2','-I',str(ROOT/'samples'),str(ROOT/'samples/stage70_runtime.c'),str(ROOT/'samples/stage70_runtime_test.c'),'-o',str(R/'self-runtime-test')]
    q=subprocess.run(cmd,capture_output=True,text=True,timeout=20);(E/'stage70-self-runtime-build.stderr.log').write_text(q.stderr);assert q.returncode==0,q.stderr
    q=subprocess.run([str(R/'self-runtime-test')],capture_output=True,text=True,timeout=10)
    (E/'stage70-self-runtime.stdout.log').write_text(q.stdout);(E/'stage70-self-runtime.stderr.log').write_text(q.stderr);assert q.returncode==0
    print(json.dumps(dict(instruction_positive_tests=len(cases),negative_tests=7,REP_flag_CFG_proven=True,native_own_helper_self_cases=json.loads(q.stdout),no_original_or_Unicorn_loaded=True)),flush=True)
if __name__=='__main__':main()
