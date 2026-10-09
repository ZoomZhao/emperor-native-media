#!/usr/bin/env python3
"""Bounded original-i386 address runtime; unsupported Windows semantics refuse.

No original instruction bytes are patched. Only IAT slots are rebound to explicit
platform functions. The guest OS profile is a documented single-thread Windows
XP ABI subset, not an independently validated Windows replacement.
"""
import collections,hashlib,json,struct,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
PKG=ROOT/'vendor-tools/unicorn-2.1.4'
assert hashlib.sha256((ROOT/'evidence/unicorn-2.1.4-cp37-abi3-macosx_11_0_arm64.whl').read_bytes()).hexdigest()=='d6c93e0f60328d8f4a1792af3f834137a28050fcc2305f2ec01efe8558a9844e'
assert hashlib.sha256((PKG/'unicorn/lib/libunicorn.2.dylib').read_bytes()).hexdigest()=='7207c8e3d7a63118fb0bca73e01816797fd51b1d8a39a4cbc7abfd562ee59c85'
sys.path.insert(0,str(PKG));sys.path.insert(0,str(ROOT/'samples'))
import unicorn as u
from unicorn import x86_const as x
from auto_i386_analysis import PE
assert u.__version__=='2.1.4'
SOURCE='<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe'
SENTINEL=0x62000000;THUNKS=0x61000000;GDT=0x60000000;TEB=0x60001000
STACK=0x70000000;STACK_SIZE=0x100000;SP=STACK+STACK_SIZE-0x100
HEAP=0x10000000;HEAP_SIZE=0x1000000;SCRATCH=0x20000000
GUEST_NAME=b'C:\\Emperor\\save-input.sav'

class Boundary(Exception):
    def __init__(self,kind,**details):self.details=dict(kind=kind,**details);super().__init__(str(self.details))

class Runtime:
    def __init__(self, save_path=None, save_sha256=None):
        print('checkpoint: load reviewed engine and original image',flush=True)
        self.pe=PE(SOURCE);assert hashlib.sha256(self.pe.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
        self.uc=u.Uc(u.UC_ARCH_X86,u.UC_MODE_32);self.pages=set();self.trace=collections.deque(maxlen=64)
        self.api_trace=[];self.visited=set();self.writes=0;self.count=0;self.last_error=0
        self.last_invalid=None
        self.heaps={};self.allocations={};self.heap_next=HEAP;self.files={};self.next_handle=0x1000
        self.critical_sections={};self.tls={};self.virtual_kernel32=0x63000000
        self.wall_clock_ns=time.time_ns();self.monotonic_origin_ns=time.monotonic_ns()
        if save_path is None:
            save_path=ROOT/'stage7-data/save-input.sav'
            save_sha256='e002894ce7dfa91d374715356708433afa70129bff673ce667f47b3520a12845'
        else:
            assert save_sha256 and Path(save_path).resolve().is_relative_to(ROOT.resolve())
        self.save=Path(save_path).read_bytes()
        assert hashlib.sha256(self.save).hexdigest()==save_sha256
        self.map(self.pe.base,4096,u.UC_PROT_READ);self.uc.mem_write(self.pe.base,self.pe.data[:4096])
        for s in self.pe.sections:
            perms=u.UC_PROT_READ|(u.UC_PROT_EXEC if s['flags']&0x20000000 else 0)|(u.UC_PROT_WRITE if s['flags']&0x80000000 else 0)
            self.map(s['va'],max(s['size'],s['rawsize']),perms)
            self.uc.mem_write(s['va'],self.pe.data[s['rawptr']:s['rawptr']+s['rawsize']])
        print('checkpoint: original image and BSS mapped',flush=True)
        for base,size,perm in [(STACK,STACK_SIZE,u.UC_PROT_READ|u.UC_PROT_WRITE),(HEAP,HEAP_SIZE,u.UC_PROT_READ|u.UC_PROT_WRITE),(SCRATCH,0x40000,u.UC_PROT_READ|u.UC_PROT_WRITE),(GDT,8192,u.UC_PROT_READ|u.UC_PROT_WRITE),(THUNKS,65536,u.UC_PROT_READ|u.UC_PROT_EXEC),(SENTINEL,4096,u.UC_PROT_READ|u.UC_PROT_EXEC)]:self.map(base,size,perm)
        # One real segmented FS domain, separate from guest address zero.
        self.uc.mem_write(GDT+8,self.descriptor(0,0xfffff,0x9b,0xc))
        self.uc.mem_write(GDT+16,self.descriptor(0,0xfffff,0x93,0xc))
        self.uc.mem_write(GDT+24,self.descriptor(TEB,4095,0x93,0x4))
        self.uc.reg_write(x.UC_X86_REG_GDTR,(0,GDT,31,0))
        self.uc.reg_write(x.UC_X86_REG_CS,8)
        for reg in (x.UC_X86_REG_DS,x.UC_X86_REG_ES,x.UC_X86_REG_SS):self.uc.reg_write(reg,16)
        self.uc.reg_write(x.UC_X86_REG_FS,24)
        self.word(TEB,0xffffffff);self.word(TEB+4,STACK+STACK_SIZE);self.word(TEB+8,STACK);self.word(TEB+0x18,TEB)
        self.uc.mem_write(SENTINEL,b'\xf4');self.imports={}
        meta=json.loads((ROOT/'evidence/pe-metadata.json').read_text())[0];i=0
        for dll in meta['imports']:
            for sym in dll['symbols']:
                address=THUNKS+i*16;i+=1
                name=sym.get('name',f"ordinal:{sym.get('ordinal')}")
                self.imports[address]=(dll['dll'],name)
                self.word(int(sym['iat_va'],16),address);self.uc.mem_write(address,b'\x0f\x0b')
        self.code_hook_handle=self.uc.hook_add(u.UC_HOOK_CODE,self.code_hook)
        self.write_hook_handle=self.uc.hook_add(u.UC_HOOK_MEM_WRITE,self.write_hook)
        self.invalid_hook_handle=self.uc.hook_add(u.UC_HOOK_MEM_INVALID,self.invalid_hook)
        self.uc.reg_write(x.UC_X86_REG_EFLAGS,0x202)
        print('checkpoint: FS, stack, heap and explicit IAT boundaries ready',flush=True)
    @staticmethod
    def descriptor(base,limit,access,flags):
        return struct.pack('<HHBBBB',limit&65535,base&65535,(base>>16)&255,access,((limit>>16)&15)|(flags<<4),(base>>24)&255)
    def map(self,base,size,permissions):
        # Map contiguous spans in one call. Thousands of separate mappings made
        # full-image initialization exceed the bounded process budget.
        pending=[p for p in range(base&~4095,(base+size+4095)&~4095,4096) if p not in self.pages]
        while pending:
            start=pending[0];n=1
            while n<len(pending) and pending[n]==start+n*4096:n+=1
            self.uc.mem_map(start,n*4096,permissions)
            self.pages.update(pending[:n]);pending=pending[n:]
    def word(self,addr,value):self.uc.mem_write(addr,struct.pack('<I',value&0xffffffff))
    def readword(self,addr):return struct.unpack('<I',bytes(self.uc.mem_read(addr,4)))[0]
    def cstring(self,addr,limit=4096):
        result=bytearray()
        for i in range(limit):
            b=bytes(self.uc.mem_read(addr+i,1))[0]
            if b==0:return bytes(result)
            result.append(b)
        raise Boundary('unterminated_guest_string',address=hex(addr))
    def args(self,n):return [self.readword(self.uc.reg_read(x.UC_X86_REG_ESP)+4+i*4) for i in range(n)]
    def ret(self,value,n):
        sp=self.uc.reg_read(x.UC_X86_REG_ESP);target=self.readword(sp)
        self.uc.reg_write(x.UC_X86_REG_EAX,value&0xffffffff)
        self.uc.reg_write(x.UC_X86_REG_ESP,sp+4+n*4);self.uc.reg_write(x.UC_X86_REG_EIP,target)
    def allocate(self,heap,flags,size):
        if heap not in self.heaps:raise Boundary('invalid_platform_heap',handle=hex(heap))
        if flags&~0x9:raise Boundary('unsupported_HeapAlloc_flags',flags=hex(flags))
        if flags&4:raise Boundary('HeapAlloc_exception_semantics_unimplemented')
        actual=max(8,(size+7)&~7);p=self.heap_next
        if p+actual>HEAP+HEAP_SIZE:self.last_error=8;return 0
        self.heap_next+=actual;self.allocations[p]=(heap,size)
        self.uc.mem_write(p,bytes(actual) if flags&8 else bytes([0xcd])*actual)
        return p
    def api(self,dll,name):
        if dll.lower()!='kernel32.dll':raise Boundary('unimplemented_API',dll=dll,name=name)
        nargs={'HeapCreate':3,'HeapAlloc':3,'HeapFree':3,'GetVersionExA':1,'GetEnvironmentVariableA':3,'GetFullPathNameA':4,'CreateFileA':7,'ReadFile':5,'CloseHandle':1,'GetLastError':0,'InterlockedIncrement':1,'InterlockedDecrement':1,'GetFileType':1,'SetFilePointer':4,'InitializeCriticalSection':1,'EnterCriticalSection':1,'LeaveCriticalSection':1,'DeleteCriticalSection':1,'TlsAlloc':0,'TlsSetValue':2,'TlsGetValue':1,'TlsFree':1,'GetCurrentThreadId':0,'SetLastError':1,'GetModuleHandleA':1,'GetProcAddress':2,'IsProcessorFeaturePresent':1,'GetSystemTimeAsFileTime':1,'GetCurrentProcessId':0,'GetTickCount':0,'QueryPerformanceCounter':1,'GetStartupInfoA':1,'GetStdHandle':1,'SetHandleCount':1}.get(name)
        if nargs is None:raise Boundary('unimplemented_API',dll=dll,name=name,guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))))
        a=self.args(nargs);self.api_trace.append(dict(dll=dll,name=name,args=[hex(v) for v in a],from_pc=hex(self.trace[-2]) if len(self.trace)>1 else None))
        if name=='HeapCreate':
            if a[0]&~1 or a[2]!=0:raise Boundary('unsupported_HeapCreate_contract',args=a)
            value=0x3000+len(self.heaps);self.heaps[value]=dict(options=a[0]);self.ret(value,nargs)
        elif name=='HeapAlloc':self.ret(self.allocate(*a),nargs)
        elif name=='HeapFree':
            heap,flags,p=a
            if flags&~1 or self.allocations.get(p,(None,))[0]!=heap:raise Boundary('invalid_HeapFree_contract',args=a)
            del self.allocations[p];self.ret(1,nargs)
        elif name=='GetVersionExA':
            if self.readword(a[0])!=148:raise Boundary('unsupported_OSVERSIONINFO_size')
            self.uc.mem_write(a[0],struct.pack('<5I',148,5,1,2600,2)+bytes(128));self.ret(1,nargs)
        elif name=='GetEnvironmentVariableA':
            self.cstring(a[0]);self.last_error=203;self.ret(0,nargs)
        elif name=='GetFullPathNameA':
            name_in=self.cstring(a[0])
            if name_in.lower()!=GUEST_NAME.lower():raise Boundary('path_outside_fixed_input',path=name_in.decode('latin1'))
            if a[1]<=len(GUEST_NAME):self.ret(len(GUEST_NAME)+1,nargs)
            else:
                self.uc.mem_write(a[2],GUEST_NAME+b'\0')
                if a[3]:self.word(a[3],a[2]+GUEST_NAME.rfind(b'\\')+1)
                self.ret(len(GUEST_NAME),nargs)
        elif name=='CreateFileA':
            filename=self.cstring(a[0])
            if filename.lower()!=GUEST_NAME.lower():raise Boundary('path_outside_fixed_input',path=filename.decode('latin1'))
            if a[1]!=0x80000000 or a[4]!=3 or a[5]!=0x80 or a[6]!=0:raise Boundary('unsupported_file_open_contract',args=a)
            if a[2]&~7:raise Boundary('unsupported_file_sharing',flags=a[2])
            if a[3] and (self.readword(a[3])!=12 or self.readword(a[3]+4)!=0):raise Boundary('unsupported_file_security')
            if self.files:raise Boundary('multiple_handle_sharing_unimplemented')
            h=self.next_handle;self.next_handle+=1;self.files[h]=dict(position=0);self.ret(h,nargs)
        elif name=='ReadFile':
            h,buffer,n,out,overlap=a
            if overlap or not out or h not in self.files:raise Boundary('unsupported_ReadFile_contract',args=a)
            at=self.files[h]['position'];chunk=self.save[at:at+n]
            if chunk:self.uc.mem_write(buffer,chunk)
            self.files[h]['position']+=len(chunk);self.word(out,len(chunk));self.ret(1,nargs)
        elif name=='CloseHandle':
            if a[0] not in self.files:raise Boundary('invalid_CloseHandle',handle=hex(a[0]))
            del self.files[a[0]];self.ret(1,nargs)
        elif name=='GetLastError':self.ret(self.last_error,nargs)
        elif name=='GetModuleHandleA':
            module=self.cstring(a[0]).lower() if a[0] else b''
            if not module:self.ret(self.pe.base,nargs)
            elif module in (b'kernel32',b'kernel32.dll'):self.ret(self.virtual_kernel32,nargs)
            else:raise Boundary('unimplemented_module',module=module.decode('latin1'))
        elif name=='GetProcAddress':
            if a[0]!=self.virtual_kernel32 or a[1]<65536:raise Boundary('unsupported_dynamic_export_contract',args=a)
            symbol=self.cstring(a[1]).decode('ascii')
            if symbol!='IsProcessorFeaturePresent':raise Boundary('unimplemented_dynamic_export',symbol=symbol)
            address=next((p for p,(dll,s) in self.imports.items() if s==symbol),None)
            if address is None:
                address=THUNKS+len(self.imports)*16;self.imports[address]=('KERNEL32.dll',symbol);self.uc.mem_write(address,b'\x0f\x0b')
            self.ret(address,nargs)
        elif name=='IsProcessorFeaturePresent':
            if a[0]!=0:raise Boundary('unsupported_processor_feature',feature=a[0])
            # Feature 0 asks specifically about the historic Pentium FDIV erratum.
            # This bounded guest CPU profile does not model that erratum.
            self.ret(0,nargs)
        elif name=='SetLastError':self.last_error=a[0];self.ret(0,nargs)
        elif name=='GetCurrentThreadId':self.ret(1,nargs)
        elif name=='GetCurrentProcessId':self.ret(2,nargs)
        elif name=='SetHandleCount':
            # The Windows NT-family API ignores the request and returns its input.
            self.ret(a[0],nargs)
        elif name=='GetStartupInfoA':
            # GUI process profile, no inherited standard handles or reserved CRT data.
            self.uc.mem_write(a[0],struct.pack('<I',68)+bytes(64));self.ret(self.uc.reg_read(x.UC_X86_REG_EAX),nargs)
        elif name=='GetStdHandle':
            if a[0] not in (0xfffffff6,0xfffffff5,0xfffffff4):raise Boundary('invalid_standard_handle_selector')
            self.ret(0,nargs)
        elif name=='GetSystemTimeAsFileTime':
            value=self.wall_clock_ns//100+116444736000000000
            self.uc.mem_write(a[0],struct.pack('<Q',value));self.ret(self.uc.reg_read(x.UC_X86_REG_EAX),nargs)
        elif name=='GetTickCount':self.ret((time.monotonic_ns()-self.monotonic_origin_ns)//1000000,nargs)
        elif name=='QueryPerformanceCounter':
            self.uc.mem_write(a[0],struct.pack('<Q',time.monotonic_ns()-self.monotonic_origin_ns));self.ret(1,nargs)
        elif name=='TlsAlloc':
            index=next(i for i in range(64) if i not in self.tls);self.tls[index]=0;self.ret(index,nargs)
        elif name in ('TlsSetValue','TlsGetValue','TlsFree'):
            if a[0] not in self.tls:raise Boundary('invalid_platform_TLS_index',index=a[0])
            if name=='TlsSetValue':self.tls[a[0]]=a[1];self.ret(1,nargs)
            elif name=='TlsGetValue':self.last_error=0;self.ret(self.tls[a[0]],nargs)
            else:del self.tls[a[0]];self.ret(1,nargs)
        elif name=='InitializeCriticalSection':
            if a[0] in self.critical_sections:raise Boundary('critical_section_reinitialization')
            self.uc.mem_read(a[0],24)
            self.critical_sections[a[0]]=0;self.ret(self.uc.reg_read(x.UC_X86_REG_EAX),nargs)
        elif name in ('EnterCriticalSection','LeaveCriticalSection','DeleteCriticalSection'):
            p=a[0]
            if p not in self.critical_sections:raise Boundary('uninitialized_critical_section',address=hex(p),api=name)
            depth=self.critical_sections[p]
            if name=='EnterCriticalSection':self.critical_sections[p]=depth+1
            elif name=='LeaveCriticalSection':
                if depth<1:raise Boundary('critical_section_not_owned',address=hex(p))
                self.critical_sections[p]=depth-1
            else:
                if depth:raise Boundary('delete_owned_critical_section',address=hex(p))
                del self.critical_sections[p]
            self.ret(self.uc.reg_read(x.UC_X86_REG_EAX),nargs)
        elif name.startswith('Interlocked'):
            v=(self.readword(a[0])+(1 if name=='InterlockedIncrement' else -1))&0xffffffff;self.word(a[0],v);self.ret(v,nargs)
        elif name=='GetFileType':
            if a[0] in (0,0xffffffff):self.last_error=6;self.ret(0,nargs)
            elif a[0] not in self.files:raise Boundary('invalid_GetFileType')
            else:self.ret(1,nargs)
        elif name=='SetFilePointer':
            h,low,high,origin=a
            if h not in self.files or high or origin not in (0,1,2):raise Boundary('unsupported_seek_contract',args=a)
            signed=low if low<0x80000000 else low-0x100000000
            at=(0 if origin==0 else self.files[h]['position'] if origin==1 else len(self.save))+signed
            if at<0:self.last_error=131;self.ret(0xffffffff,nargs)
            else:self.files[h]['position']=at;self.ret(at,nargs)
    def code_hook(self,uc,address,size,user):
        if address==SENTINEL:uc.emu_stop();return
        self.trace.append(address);self.count+=1;self.visited.add(address)
        if self.count>500000:raise Boundary('instruction_budget')
        if address in self.imports:self.api(*self.imports[address]);return
        s=self.pe.section(address)
        if not s or not s['flags']&0x20000000 or address+size>s['va']+s['size']:raise Boundary('invalid_code_target',address=hex(address))
        if address==0x7688ba:raise Boundary('forbidden_complete_EXE_entry')
        if address in (0x77a9e9,):raise Boundary('unimplemented_exception_dispatch',address=hex(address))
    def write_hook(self,uc,access,address,size,value,user):self.writes+=1
    def invalid_hook(self,uc,access,address,size,value,user):
        self.last_invalid=dict(kind='unmapped_or_protected_guest_memory',access=access,address=hex(address),size=size,pc=hex(uc.reg_read(x.UC_X86_REG_EIP)))
        return False
    def call(self,entry,args=(),this=0):
        print('checkpoint: original call '+hex(entry),flush=True)
        self.count=0;self.trace.clear();old_writes=self.writes;before_apis=len(self.api_trace)
        self.last_invalid=None
        sp=SP-4*(len(args)+1);self.uc.mem_write(sp,struct.pack('<'+str(len(args)+1)+'I',SENTINEL,*args))
        self.uc.reg_write(x.UC_X86_REG_ESP,sp);self.uc.reg_write(x.UC_X86_REG_ECX,this)
        item=dict(entry=hex(entry),args=[hex(a) for a in args],this=hex(this))
        try:
            self.uc.emu_start(entry,SENTINEL,timeout=1000000,count=500000)
            if self.uc.reg_read(x.UC_X86_REG_EIP)!=SENTINEL:raise Boundary('engine_budget_before_return')
            item.update(status='returned',eax=self.uc.reg_read(x.UC_X86_REG_EAX),esp=hex(self.uc.reg_read(x.UC_X86_REG_ESP)),seh_head=hex(self.readword(TEB)))
        except (Boundary,u.UcError) as e:
            item.update(status='refused',boundary=e.details if isinstance(e,Boundary) else (self.last_invalid or dict(kind='Unicorn_error',text=str(e))))
            item['registers']={n:hex(self.uc.reg_read(reg)) for n,reg in [('eax',x.UC_X86_REG_EAX),('ecx',x.UC_X86_REG_ECX),('edx',x.UC_X86_REG_EDX),('ebx',x.UC_X86_REG_EBX),('esp',x.UC_X86_REG_ESP),('ebp',x.UC_X86_REG_EBP),('esi',x.UC_X86_REG_ESI),('edi',x.UC_X86_REG_EDI),('eip',x.UC_X86_REG_EIP)]}
        item.update(instructions=self.count,write_events=self.writes-old_writes,api_calls=self.api_trace[before_apis:],last_trace=[hex(v) for v in self.trace])
        return item
