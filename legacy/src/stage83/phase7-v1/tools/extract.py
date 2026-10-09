"""Reuse task-authored finite kernel verbatim; retain source provenance."""
from core import *
import ast
src=ROOT/'samples/stage20_threads.py';text=src.read_text();tree=ast.parse(text);node=next(n for n in tree.body if isinstance(n,ast.ClassDef)and n.name=='ThreadKernel');body=ast.get_source_segment(text,node)
header='import copy,struct,time\nfrom unicorn import x86_const as x\nimport unicorn as u\nfrom core import Boundary\nGDT=0x60000000;TEB=0x60001000;STACK=0x70000000;STACK_SIZE=0x100000;SENTINEL=0x62000000\n'
(V/'tools/thread_kernel.py').write_text(header+body+'\n')
put(V/'evidence/thread-kernel-provenance.json',{'source':str(src.relative_to(ROOT)),'source_sha256':sha(src),'class_body_sha256':hashlib.sha256(body.encode()).hexdigest(),'class_body_verbatim':True,'dependencies_replaced_only':True,'activation_not_called_for_restore':True,'original_execution':0})
