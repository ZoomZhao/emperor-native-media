#!/usr/bin/env python3
"""Bounded PE/i386 address and flag-provenance prototype (stdlib only).

Inputs are original bytes, reviewed code windows and entry points. No previous
SCI, map.json, relocation list or translated output is read. Unsupported opcodes,
indirect control flow and code-boundary conflicts are explicit unknowns.
"""
from pathlib import Path
import struct,json,hashlib,collections,sys
ALL=0x3f; LOGIC=0x3b
# SR compact flags are CF, PF, AF, ZF, SF, OF (not architectural EFLAGS).
MUL_FLAGS=0x21
CC_FLAGS=(0x20,0x20,1,1,8,8,9,9,0x10,0x10,2,2,0x30,0x30,0x38,0x38)
REGS=['eax','ecx','edx','ebx','esp','ebp','esi','edi']
class PE:
    def __init__(self,path):
        self.path=path;self.data=Path(path).read_bytes();p=struct.unpack_from('<I',self.data,0x3c)[0]
        assert self.data[p:p+4]==b'PE\0\0'
        machine,n,_,_,_,optsize,_=struct.unpack_from('<HHIIIHH',self.data,p+4);assert machine==0x14c
        opt=p+24;assert struct.unpack_from('<H',self.data,opt)[0]==0x10b
        self.base=struct.unpack_from('<I',self.data,opt+28)[0];self.sections=[]
        for i in range(n):
            name,vs,rva,rawsize,rawptr,_,_,_,_,flags=struct.unpack_from('<8sIIIIIIHHI',self.data,opt+optsize+i*40)
            self.sections.append(dict(name=name.rstrip(b'\0').decode(),va=self.base+rva,size=vs,rawsize=rawsize,rawptr=rawptr,flags=flags))
    def section(self,va):return next((s for s in self.sections if s['va']<=va<s['va']+s['size']),None)
    def read(self,va,n):
        s=self.section(va)
        if not s or va+n>s['va']+s['rawsize']:raise ValueError('No file-backed bytes at '+hex(va))
        off=s['rawptr']+va-s['va'];return self.data[off:off+n]
def reg(i,bits=32):
    # ModRM encodings4..7 at byte width denote AH/CH/DH/BH, not ESP..EDI.
    if bits==8 and i>=4:return dict(kind='reg',reg=i-4,bits=8,shift=8)
    return dict(kind='reg',reg=i,bits=bits)
def imm(value,site=None,width=0):return dict(kind='imm',value=value,site=site,width=width)
def signed(v,bits):return v-(1<<bits) if v&(1<<(bits-1)) else v
def decode(pe,pc):
    start=pc
    operand_bits=32;repeat=False
    def get(n=1):
        nonlocal pc
        value=int.from_bytes(pe.read(pc,n),'little');pc+=n;return value
    def operand(bits=None):
        if bits is None:bits=operand_bits
        b=get();mode,index,rm=b>>6,(b>>3)&7,b&7
        if mode==3:return reg(rm,bits),index
        base=rm;idx=None;scale=1;disp=0;site=None;width=0
        if rm==4:
            sib=get();base=sib&7;idx=(sib>>3)&7;scale=1<<(sib>>6)
            if idx==4:idx=None
        if mode==0 and base==5:base=None;site=pc;width=4;disp=get(4)
        elif mode==1:site=pc;width=1;disp=signed(get(),8)
        elif mode==2:site=pc;width=4;disp=signed(get(4),32)
        return dict(kind='mem',base=base,index=idx,scale=scale,disp=disp,site=site,width=width,bits=bits),index
    op=get();prefixes=[]
    while op in (0x66,0xf3):
        if op in prefixes:raise ValueError('Duplicate prefix outside supported contract')
        prefixes.append(op)
        if op==0x66:operand_bits=16
        else:repeat=True
        op=get()
    name=None;args=[];defines=0;undefined=0;control=None;target=None;effects={}
    if repeat and op not in (0xaa,0xab):raise ValueError('REP only supported for STOS; no guessed prefix semantics')
    if op in (0xaa,0xab):
        bits=8 if op==0xaa else operand_bits;name='rep_stos' if repeat else 'stos'
        args=[dict(kind='mem',base=7,index=None,scale=1,disp=0,site=None,width=0,bits=bits,segment='es'),reg(0,bits)]
        effects=dict(read_registers=['eax','edi']+(['ecx'] if repeat else []),write_registers=['edi']+(['ecx']if repeat else []),
            input_architectural_flags=0x400,preserved_arithmetic_flags=ALL,element_bits=bits,address_bits=32,
            repeat_count_register='ecx'if repeat else None,per_element_commit_required=True,segment_base_required='es')
    elif op in (0xfc,0xfd):name='cld'if op==0xfc else 'std';effects=dict(direction_flag=0 if op==0xfc else 1,preserved_arithmetic_flags=ALL)
    elif 0xb8<=op<=0xbf:name='mov';site=pc;n=operand_bits//8;args=[reg(op-0xb8,operand_bits),imm(get(n),site,n)]
    elif 0x50<=op<=0x57:name='push';args=[reg(op-0x50,operand_bits)]
    elif 0x58<=op<=0x5f:name='pop';args=[reg(op-0x58,operand_bits)]
    elif 0x40<=op<=0x4f:name='inc' if op<0x48 else 'dec';args=[reg(op&7,operand_bits)];defines=ALL&~1
    elif op in (0x68,0x6a):
        name='push';site=pc;width=operand_bits//8 if op==0x68 else 1;v=get(width);args=[imm(v if op==0x68 else signed(v,8),site,width)]
    elif op==0x99:name='cdq'
    elif op==0x90:name='nop'
    elif op in (0xe8,0xe9,0xeb):
        name='call' if op==0xe8 else 'jmp';n=1 if op==0xeb else 4;delta=signed(get(n),n*8);target=pc+delta;control=name
    elif op in (0xc3,0xc2):name='ret';control='ret';args=[imm(get(2) if op==0xc2 else 0)]
    elif 0x70<=op<=0x7f:
        delta=signed(get(),8);name='jcc';control='jcc';target=pc+delta;args=[imm(op&15)]
    elif op==0x0f:
        second=get()
        if 0x80<=second<=0x8f:delta=signed(get(4),32);name='jcc';control='jcc';target=pc+delta;args=[imm(second&15)]
        elif 0x90<=second<=0x9f:m,group=operand(8);name='setcc';args=[m,imm(second&15)]
        elif second in (0xbe,0xbf,0xb6,0xb7):
            m,r=operand(8 if second in (0xbe,0xb6) else 16);name='movsx'if second in (0xbe,0xbf)else'movzx';args=[reg(r,operand_bits),m]
        elif second==0xaf:m,r=operand();name='imul';args=[reg(r,operand_bits),m];defines=MUL_FLAGS;undefined=ALL^MUL_FLAGS
        else:raise ValueError('Unsupported 0f opcode '+hex(second))
    elif op in (0x3c,0x3d):name='cmp';site=pc;n=1 if op==0x3c else operand_bits//8;args=[reg(0,n*8),imm(get(n),site,n)]
    elif op in (0xc1,0xd1):
        m,group=operand();name={4:'shl',5:'shr',7:'sar'}.get(group)
        if name is None:raise ValueError('Unsupported shift group')
        count=get() if op==0xc1 else 1;args=[m,imm(count)];count&=31
        if count:defines=LOGIC if count==1 else LOGIC&~0x20;undefined=4|(0x20 if count>1 else 0)
    elif op in (0xf6,0xf7):
        bits=8 if op==0xf6 else operand_bits;m,group=operand(bits)
        if group==0:
            name='test';site=pc;n=bits//8;args=[m,imm(get(n),site,n)]
        else:name={3:'neg',5:'imul',7:'idiv'}.get(group)
        if name is None:raise ValueError('Unsupported f7 group')
        if group!=0:args=[m];defines=ALL if group==3 else MUL_FLAGS if group==5 else 0;undefined=0 if group==3 else ALL^MUL_FLAGS if group==5 else ALL
    elif op in (0xa0,0xa1,0xa2,0xa3):
        name='mov';bits=8 if op in (0xa0,0xa2) else operand_bits;site=pc;addr=get(4)
        m=dict(kind='mem',base=None,index=None,scale=1,disp=addr,site=site,width=4,bits=bits)
        args=[reg(0,bits),m] if op in (0xa0,0xa1) else [m,reg(0,bits)]
    elif op in (0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7):name='mov';site=pc;args=[reg(op-0xb0,8),imm(get(),site,1)]
    elif op in (0x8b,0x8a,0x89,0x88,0x8d,0x33,0x32,0x03,0x2b,0x3b,0x85,0x84,0x0b,0x0a):
        bits=8 if op in (0x8a,0x88,0x32,0x84,0x0a) else operand_bits;m,r=operand(bits)
        name={0x8b:'mov',0x8a:'mov',0x89:'mov',0x88:'mov',0x8d:'lea',0x33:'xor',0x32:'xor',0x03:'add',0x2b:'sub',0x3b:'cmp',0x85:'test',0x84:'test',0x0b:'or',0x0a:'or'}[op]
        args=[m,reg(r,bits)] if op in (0x89,0x88,0x85,0x84) else [reg(r,bits),m]
    elif op in (0x83,0x81,0x80,0xc6,0xc7):
        bits=8 if op in (0x80,0xc6) else operand_bits;m,group=operand(bits);site=pc;width=1 if op in (0x80,0x83,0xc6) else bits//8;v=get(width)
        if op==0x83:v=signed(v,8)&0xffffffff
        if op in (0xc6,0xc7):assert group==0;name='mov'
        else:name={0:'add',1:'or',4:'and',5:'sub',6:'xor',7:'cmp'}.get(group)
        if name is None:raise ValueError('Unsupported group '+str(group))
        args=[m,imm(v,site,width)]
    elif op==0xff:
        m,group=operand()
        if group in (2,4):name='call_indirect' if group==2 else 'jmp_indirect';control=name;args=[m]
        elif group==6:name='push';args=[m]
        else:raise ValueError('Unsupported ff group '+str(group))
    else:raise ValueError('Unsupported opcode '+hex(op))
    if name in ('add','sub','cmp'):defines=ALL
    elif name in ('test','xor','or','and'):defines=LOGIC;undefined=4
    if operand_bits==16 and name in ('call','jmp','ret','jcc'):raise ValueError('16-bit control transfer outside supported contract')
    effects.update(operand_bits=operand_bits)
    return dict(pc=start,end=pc,name=name,args=args,defines=defines,undefined=undefined,control=control,target=target,bytes=pe.read(start,pc-start).hex(),effects=effects)

class Analysis:
    def __init__(self,pe,config):
        self.pe=pe;self.config=config;self.windows=[tuple(int(v,0) if isinstance(v,str) else v for v in pair) for pair in config['windows']]
        self.roots=[int(v,0) if isinstance(v,str) else v for v in config['roots']]
        self.initializers=[int(v,0) if isinstance(v,str) else v for v in config.get('initializers',[])]
        self.instructions={};self.unknown=[];self.candidates={};self.memory={};self.stores=[];self.used_origins=set();self.used_slots=set();self.trace_stops=[];self.functions=set(self.roots+self.initializers)
    def allowed(self,pc):return any(lo<=pc<hi for lo,hi in self.windows)
    def record_unknown(self,kind,pc,**info):
        item=dict(kind=kind,address=hex(pc),**info)
        if item not in self.unknown:self.unknown.append(item)
    def successors(self,ins):
        c=ins['control']
        if c in ('ret','jmp_indirect'):return []
        if c=='jmp':return [ins['target']]
        if c=='jcc':return [ins['end'],ins['target']]
        return [ins['end']]
    def scan(self):
        queue=collections.deque(self.roots+self.initializers);owned={}
        while queue:
            pc=queue.popleft()
            if pc in self.instructions:continue
            if len(self.instructions)>=512:self.record_unknown('instruction_budget',pc);break
            if not self.allowed(pc):self.record_unknown('outside_reviewed_code_window',pc);continue
            if not self.pe.section(pc) or not self.pe.section(pc)['flags']&0x20000000:self.record_unknown('entry_in_nonexecutable_section',pc);continue
            if pc in owned:self.record_unknown('branch_into_instruction_middle',pc,owner=hex(owned[pc]));continue
            try:ins=decode(self.pe,pc)
            except (ValueError,AssertionError,IndexError) as e:self.record_unknown('decoder_unsupported',pc,reason=str(e));continue
            if not all(self.allowed(x) for x in range(pc,ins['end'])):self.record_unknown('instruction_crosses_code_window',pc);continue
            if any(x in owned for x in range(pc,ins['end'])):self.record_unknown('overlapping_code_decode',pc);continue
            self.instructions[pc]=ins
            for x in range(pc,ins['end']):owned[x]=pc
            for arg in ins['args']:
                field=arg.get('site');width=arg.get('width')
                if field is None or width!=4:continue
                value=arg.get('value',arg.get('disp',0))&0xffffffff;s=self.pe.section(value)
                if not s:
                    if arg['kind']=='mem' and arg['base'] is None:self.record_unknown('absolute_memory_outside_image',pc,target=hex(value))
                    continue
                absolute=arg['kind']=='mem' and arg['base'] is None
                self.candidates[field]=dict(site=hex(field),instruction=hex(pc),target=hex(value),operand_kind=arg['kind'],confidence='high' if absolute else 'pending',evidence=['decoded absolute memory-address operand' if absolute else 'mapped-image value alone cannot distinguish integer from pointer'],target_section=s['name'],source='original_file_operand',initializer_scope=False)
            if ins['control']=='call':
                self.functions.add(ins['target']);queue.append(ins['target'])
            if ins['control'] in ('call_indirect','jmp_indirect'):self.record_unknown('indirect_control_target',pc,operand=ins['args'][0])
            queue.extend(self.successors(ins))
        initializer_nodes=set()
        for root in self.initializers:initializer_nodes.update(self.reachable(root,include_calls=True))
        for c in self.candidates.values():c['initializer_scope']=int(c['instruction'],16) in initializer_nodes
    def reachable(self,entry,include_calls=False):
        seen=set();q=[entry]
        while q:
            pc=q.pop()
            if pc in seen or pc not in self.instructions:continue
            seen.add(pc);ins=self.instructions[pc];q+=self.successors(ins)
            if include_calls and ins['control']=='call':q.append(ins['target'])
        return seen
    def flag_summary(self,entry,summaries):
        incoming=tuple(frozenset(['incoming']) for _ in range(6));states={entry:incoming};q=collections.deque([entry]);returns=[];budget=0
        while q:
            budget+=1
            if budget>10000:self.record_unknown('flag_fixpoint_budget',entry);break
            pc=q.popleft()
            if pc not in self.instructions:self.record_unknown('flags_unknown_successor',pc);continue
            ins=self.instructions[pc];state=list(states[pc])
            for bit in range(6):
                if ins['defines']&(1<<bit):state[bit]=frozenset([pc])
                elif ins['undefined']&(1<<bit):state[bit]=frozenset(['undefined'])
            if ins['control'] in ('call','call_indirect'):
                summary=summaries.get(ins['target']) if ins['control']=='call' else None
                if summary is None:state=[frozenset(['unknown-call@'+hex(pc)]) for _ in range(6)]
                else:
                    state=[frozenset(x for x in summary[b] if x!='incoming')|(state[b] if 'incoming' in summary[b] else frozenset()) for b in range(6)]
            state=tuple(state)
            if ins['control']=='ret':returns.append(state)
            for nxt in self.successors(ins):
                prior=states.get(nxt);merged=state if prior is None else tuple(prior[b]|state[b] for b in range(6))
                if merged!=prior:states[nxt]=merged;q.append(nxt)
        if not hasattr(self,'flag_states'):self.flag_states={}
        self.flag_states[entry]=states
        if not returns:return None
        return tuple(frozenset().union(*(r[b] for r in returns)) for b in range(6))
    def flags(self):
        summaries={}
        for _ in range(12):
            new={f:self.flag_summary(f,summaries) for f in self.functions}
            if new==summaries:break
            summaries=new
        else:self.record_unknown('call_summary_round_budget',0)
        producers=collections.defaultdict(int);contracts=[]
        for root in self.roots:
            summary=summaries.get(root)
            if summary is None:self.record_unknown('no_closed_return_for_flags',root);continue
            for bit,origins in enumerate(summary):
                for origin in origins:
                    if isinstance(origin,int):producers[origin]|=1<<bit
                    elif origin!='undefined':contracts.append(dict(entry=hex(root),flag_bit=bit,provenance=origin))
        emitted={};proof=[];consumer_inputs={};consumer_proof=[]
        leaders=set(self.functions)
        for ins in self.instructions.values():
            if ins['control'] in ('jmp','jcc'):leaders.add(ins['target'])
            if ins['control'] is not None:leaders.add(ins['end'])
        leaders=sorted(leaders)
        # SR analyses each region backwards independently. A later conditional
        # can reuse a comparison from another region. Prove every conditional
        # read from the original CFG, force its producers to materialise flags,
        # and declare those proved incoming flags at the consumer. This is
        # generic dataflow metadata, never an address-specific game patch.
        for entry,states in self.flag_states.items():
            for pc,state in states.items():
                ins=self.instructions.get(pc)
                if ins is None or ins['name'] not in ('jcc','setcc'):continue
                mask=CC_FLAGS[ins['args'][-1]['value']]
                origins=set().union(*(state[b] for b in range(6) if mask&(1<<b)))
                if any(not isinstance(o,int) for o in origins):
                    self.record_unknown('conditional_flags_unproved',pc,entry=hex(entry),origins=[hex(o) if isinstance(o,int) else o for o in sorted(origins,key=str)])
                    continue
                region_start=max((p for p in leaders if p<=pc),default=pc)
                incoming_mask=0
                for b in range(6):
                    if mask&(1<<b):
                        for origin in state[b]:producers[origin]|=1<<b
                        if any(not region_start<=origin<pc for origin in state[b]):incoming_mask|=1<<b
                if incoming_mask:consumer_inputs[region_start]=consumer_inputs.get(region_start,0)|incoming_mask
                consumer_proof.append(dict(entry=hex(entry),consumer=hex(pc),sr_mask=hex(mask),producers=[hex(o) for o in sorted(origins)],region_start=hex(region_start),region_incoming_sr_mask=hex(incoming_mask),reason='original CFG flag provenance; all incoming paths have decoded defined producers'))
        for pc,mask in sorted(producers.items()):
            point=pc;cursor=self.instructions[pc]['end'];visited=set()
            while cursor in self.instructions and cursor not in visited:
                visited.add(cursor);ins=self.instructions[cursor]
                if ins['control']=='jcc':point=cursor;break
                if ins['control'] is not None or (ins['defines']|ins['undefined'])&mask:break
                cursor=ins['end']
            emitted[point]=emitted.get(point,0)|mask
            proof.append(dict(producer=hex(pc),mnemonic=self.instructions[pc]['name'],defined_sr_mask=hex(mask),annotation_site=hex(point),reason='last defined flag producer reaching a reviewed return boundary; conditional branch used if reached before overwrite'))
        return dict(policy='all defined arithmetic flags observed at reviewed entry returns plus proved conditional flag reads; NOT inferred game caller contract',annotations=[dict(site=hex(p),sr_mask=hex(emitted.get(p,0)),sr_input_mask=hex(consumer_inputs.get(p,0))) for p in sorted(set(emitted)|set(consumer_inputs))],producer_evidence=proof,conditional_read_evidence=consumer_proof,incoming_or_unknown_contracts=contracts)
    def trace_initializers(self):
        """Exact constants only; follows direct calls, bounded loops/branches.
        Never treats original BSS zero as immutable runtime pointer state.
        """
        def constant(value,origins=(),slots=()):return dict(value=value&0xffffffff,origins=set(origins),slots=set(slots))
        def unknown():return None
        def execute(entry,seed_memory):
            regs=[None]*8;sp=0;stack={};calls=[];pc=entry;last_cmp=None;steps=0;memory=dict(seed_memory)
            completed=False
            def address(arg):
                if arg['kind']!='mem':return None
                parts=[];v=arg['disp']
                for r,scale in ((arg['base'],1),(arg['index'],arg['scale'])):
                    if r is not None:
                        if regs[r] is None:return None
                        parts.append(regs[r]);v+=regs[r]['value']*scale
                # A mapped-image integer used as a scaled index is not proved
                # to be a rebasable pointer. Only exact constants in base regs.
                if arg['base'] is not None:
                    p=regs[arg['base']]
                    exact={o for o in p['origins'] if o in self.candidates and int(self.candidates[o]['target'],16)==p['value']}
                    self.used_origins.update(exact)
                    if exact:self.used_slots.update(p['slots'])
                return v&0xffffffff
            def get(arg):
                if arg['kind']=='imm':return constant(arg['value'],[arg['site']] if arg.get('site') else [])
                if arg['kind']=='reg':return regs[arg['reg']] if arg['bits']==32 else None
                if arg['base']==4 and arg['index'] is None:return stack.get(sp+arg['disp'])
                a=address(arg)
                if a is None:return None
                if a in memory:
                    v=memory[a];return constant(v['value'],v['origins'],v['slots']|{a})
                return None
            def put(arg,v):
                if arg['kind']=='reg':regs[arg['reg']]=v if arg['bits']==32 else None
                elif arg['base']==4 and arg['index'] is None:stack[sp+arg['disp']]=v
                else:
                    a=address(arg)
                    if a is not None and v is not None:
                        memory[a]=v;self.stores.append(dict(instruction=hex(pc),slot=hex(a),value=hex(v['value']),origins=[hex(x) for x in sorted(v['origins'])]))
            while pc in self.instructions and steps<128:
                steps+=1;ins=self.instructions[pc];name=ins['name'];args=ins['args'];nxt=ins['end']
                if name=='mov':put(args[0],get(args[1]))
                elif name=='push':sp-=4;stack[sp]=get(args[0])
                elif name=='pop':put(args[0],stack.get(sp));sp+=4
                elif name=='call':sp-=4;stack[sp]=constant(nxt);calls.append(nxt);nxt=ins['target']
                elif name=='ret':
                    if not calls:completed=True;break
                    nxt=calls.pop();sp+=4+args[0]['value']
                elif name=='jmp':nxt=ins['target']
                elif name in ('cmp','test'):
                    a,b=get(args[0]),get(args[1]);last_cmp=(name,a['value'],b['value']) if a and b else None
                elif name=='jcc':
                    if last_cmp is None:self.trace_stops.append(dict(entry=hex(entry),address=hex(pc),reason='runtime-unknown conditional; remaining constant-flow path not assumed'));break
                    cmp,a,b=last_cmp;sa,sb=signed(a,32),signed(b,32);c=args[0]['value']
                    if cmp=='test':a=a&b;b=0;sa=signed(a,32);sb=0
                    choices={4:a==b,5:a!=b,12:sa<sb,13:sa>=sb,14:sa<=sb,15:sa>sb}
                    if c not in choices:break
                    if choices[c]:nxt=ins['target']
                elif name in ('add','sub') and args[0]==reg(4) and args[1]['kind']=='imm':sp+=args[1]['value'] if name=='add' else -args[1]['value']
                elif name=='lea':
                    a=address(args[1]);put(args[0],constant(a) if a is not None else None)
                elif name in ('xor','or','and','add','sub','inc'):
                    a=get(args[0]);b=get(args[1]) if len(args)>1 else constant(1)
                    if name=='xor' and args[0]==args[1]:put(args[0],constant(0))
                    elif a and b:
                        v={'xor':a['value']^b['value'],'or':a['value']|b['value'],'and':a['value']&b['value'],'add':a['value']+b['value'],'sub':a['value']-b['value'],'inc':a['value']+1}[name]
                        # General arithmetic may convert pointers into integers;
                        # do not propagate relocation proof through it.
                        put(args[0],constant(v))
                    else:put(args[0],None)
                    last_cmp=None
                elif name=='cdq':regs[2]=None
                elif name=='nop':pass
                else:self.trace_stops.append(dict(entry=hex(entry),address=hex(pc),reason='unsupported constant-flow operation'));break
                pc=nxt
            if steps>=128:self.trace_stops.append(dict(entry=hex(entry),address=hex(pc),reason='constant-flow step budget'))
            return memory,completed
        for entry in self.initializers:
            values,completed=execute(entry,self.memory)
            if completed:self.memory=values
            else:self.record_unknown('initializer_trace_not_closed',entry);self.memory={}
        for entry in self.roots:execute(entry,self.memory)
        for origin in self.used_origins:
            if origin in self.candidates:
                c=self.candidates[origin];c['confidence']='high';c['evidence'].append('constant propagated to a decoded memory-address base during bounded direct-call trace')
        bindings=[]
        for slot,value in sorted(self.memory.items()):
            if self.pe.section(value['value']):
                used=slot in self.used_slots
                bindings.append(dict(slot=hex(slot),target=hex(value['value']),confidence='high' if used else 'pending',evidence='initializer store traced from original immediate; loaded value subsequently used as memory base' if used else 'initializer stored mapped-image value; later dereference not proved',origins=[hex(x) for x in sorted(value['origins'])],source='runtime_initializer_pointer_slot',file_relocation=False))
        return bindings
    def run(self):
        self.scan();bindings=self.trace_initializers();flags=self.flags()
        return dict(source=self.pe.path,source_sha256=hashlib.sha256(self.pe.data).hexdigest(),configuration=self.config,instruction_count=len(self.instructions),instructions=[self.instructions[k] for k in sorted(self.instructions)],address_candidates=[self.candidates[k] for k in sorted(self.candidates)],runtime_pointer_bindings=bindings,initializer_stores=self.stores,constant_trace_stops=self.trace_stops,flags=flags,unknowns=self.unknown,conversion_ready=not self.unknown and not any(str(c['provenance']).startswith('unknown') for c in flags['incoming_or_unknown_contracts']),limits=dict(instructions=512,flag_worklist_updates=10000,summary_rounds=12,constant_trace_steps=128),assumptions=['entry points and code windows require human semantic confirmation','only the supported i386 instruction subset is decoded','strict observation of defined arithmetic flags is a test policy, not a recovered caller ABI','no scan or automatic rebase of arbitrary mapped-image integers','runtime pointer slots require initialization evidence; original BSS bytes are not relocation records'])
def main():
    cfg=json.loads(Path(sys.argv[2]).read_text());result=Analysis(PE(sys.argv[1]),cfg).run();out=Path(sys.argv[3]);out.mkdir(parents=True,exist_ok=True)
    (out/'analysis.json').write_text(json.dumps(result,indent=2))
    (out/'instruction_flags.sci').write_text(''.join(f'loc_{int(a["site"],16):X},{a["sr_mask"]},{a["sr_input_mask"]}\n' for a in result['flags']['annotations']))
    (out/'address-candidates.csv').write_text('site,target,confidence,scope\n'+''.join(f'{a["site"]},{a["target"]},{a["confidence"]},{"initializer" if a["initializer_scope"] else "module"}\n' for a in result['address_candidates']))
    print(json.dumps(dict(instructions=result['instruction_count'],module_candidates=sum(not a['initializer_scope'] for a in result['address_candidates']),initializer_candidates=sum(a['initializer_scope'] for a in result['address_candidates']),runtime_bindings=len(result['runtime_pointer_bindings']),flag_annotations=len(result['flags']['annotations']),unknowns=len(result['unknowns']))))
if __name__=='__main__':main()
