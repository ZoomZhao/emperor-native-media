"""Read-only fixed PE disassembly with already installed Apple llvm-objdump."""
import hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
EXE=Path('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe')
LLVM='<LOCAL_MACHINE_PATH>'
def main():
    assert hashlib.sha256(EXE.read_bytes()).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
    folder=ROOT/'runs/stage69-static';folder.mkdir(exist_ok=True)
    cmd=[LLVM,'--disassemble','--disassemble-zeroes','--x86-asm-syntax=intel',str(EXE)]
    with (folder/'fixed-text.disasm').open('wb') as out,(ROOT/'evidence/stage69-LLVM.stderr.log').open('wb') as err:
        q=subprocess.run(cmd,stdout=out,stderr=err,timeout=45)
    record=dict(command=cmd,returncode=q.returncode,read_only_fixed_EXE=True,original_instructions_executed=0,
        file=str(folder/'fixed-text.disasm'),bytes=(folder/'fixed-text.disasm').stat().st_size)
    (ROOT/'evidence/stage69-disassembly-source.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record),flush=True)
    assert q.returncode==0
if __name__=='__main__':main()
