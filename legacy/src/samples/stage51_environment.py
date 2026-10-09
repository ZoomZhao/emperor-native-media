"""Own bounded guest-process environment; never imports the host environment."""
from stage49_readonly import normalize,PolicyError

class GuestEnvironment:
    def __init__(self,cwd=b'C:\\Emperor'):
        self.variables={}
        self.drive_cwds={'C':self._cwd(cwd)}

    @staticmethod
    def _cwd(cwd):
        parts,full=normalize(cwd)
        if parts:raise PolicyError('native cwd currently owns only the C:\\Emperor root')
        return full

    @staticmethod
    def _name(name):
        if not isinstance(name,bytes)or not name or len(name)>255 or b'\0'in name:
            raise PolicyError('unsupported environment name length or null input')
        if any(c<33 or c>126 for c in name):
            raise PolicyError('environment names currently require printable ASCII')
        key=name.upper()
        if b'='in key and key!=b'=C:':
            raise PolicyError('only observed hidden =C: name is supported')
        return key

    def set(self,name,value,last_error,cwd):
        current=self._cwd(cwd);assert self.drive_cwds=={'C':current}
        key=self._name(name)
        if value is not None:
            if not isinstance(value,bytes)or not value or len(value)>4095 or b'\0'in value:
                raise PolicyError('empty non-NULL, NUL, or oversized environment value not measured')
            try:value.decode('gbk')
            except UnicodeError:raise PolicyError('invalid guest ACP936 value')
        proposed=dict(self.variables)
        if value is None:
            if key not in proposed:raise PolicyError('deleting an absent variable is not measured')
            del proposed[key]
        else:
            if key==b'=C:':
                canonical=self._cwd(value)
                if canonical!=current:raise PolicyError('hidden drive cwd differs from real native guest cwd')
                # Preserve supplied bytes for GetEnvironmentVariableA.
            proposed[key]=value
        if sum(len(k)+len(v)+2 for k,v in proposed.items())+1>32767:
            raise PolicyError('bounded XP guest environment block exhausted')
        self.variables=proposed
        # The supported drive remains backed by the same real native directory.
        self.drive_cwds={'C':current}
        return 1,last_error

    def get(self,name,capacity,last_error):
        key=self._name(name)
        if not 0<=capacity<=4096:raise PolicyError('environment output capacity exceeds scope')
        if key not in self.variables:return 0,203,None
        value=self.variables[key]
        if capacity<=len(value):return len(value)+1,last_error,None
        return len(value),last_error,value+b'\0'

    def resolve(self,raw,cwd):
        current=self._cwd(cwd);assert self.drive_cwds=={'C':current}
        try:text=raw.decode('gbk').replace('/','\\')
        except UnicodeError:raise PolicyError('invalid ACP936 path')
        if len(text)>1 and text[1]==':' and(len(text)==2 or text[2]!='\\'):
            drive=text[0].upper()
            if drive not in self.drive_cwds:raise PolicyError('drive-relative path uses an unmounted drive')
            base=self.drive_cwds[drive].decode('gbk')
            raw=(base+('\\'+text[2:]if text[2:]else '')).encode('gbk')
        return normalize(raw,current)

    def snapshot(self,cwd):
        current=self._cwd(cwd);assert self.drive_cwds=={'C':current}
        return dict(version=1,drive_cwds=dict(self.drive_cwds),variables=sorted(self.variables.items()))

    @classmethod
    def restored(cls,saved,cwd):
        if set(saved)!=set(['version','drive_cwds','variables'])or saved['version']!=1:
            raise PolicyError('unsupported guest environment snapshot')
        obj=cls(cwd)
        if saved['drive_cwds']!=obj.drive_cwds:raise PolicyError('restored drive cwd contradicts native directory')
        seen=set()
        for name,value in saved['variables']:
            key=obj._name(name)
            if key!=name or key in seen:raise PolicyError('duplicate or noncanonical environment snapshot')
            seen.add(key);obj.set(name,value,0,cwd)
        if obj.snapshot(cwd)!=saved:raise PolicyError('noncanonical environment snapshot ordering')
        return obj

class EnvironmentMixin:
    """API glue testable without an emulator, an EXE, a GUI, or an audio device."""
    environment_extra_fields51=['environment_state51','environment_calls51']
    def guest_cwd51(self):
        status,size,error,body=self.paths.get(261,bytes(261),self.last_error)
        if status!=1:raise PolicyError('real native guest cwd unavailable')
        return body[:size]
    def ensure_environment51(self):
        if not hasattr(self,'guest_environment51'):
            self.guest_environment51=GuestEnvironment(self.guest_cwd51())
        if not hasattr(self,'environment_calls51'):self.environment_calls51=[]
        self.guest_environment51.snapshot(self.guest_cwd51())
        return self.guest_environment51
    @property
    def environment_state51(self):return self.ensure_environment51().snapshot(self.guest_cwd51())
    @environment_state51.setter
    def environment_state51(self,saved):
        self.guest_environment51=GuestEnvironment.restored(saved,self.guest_cwd51())
    def path49(self,raw):
        return self.ensure_environment51().resolve(raw,self.guest_cwd51())
    def api(self,dll,name):
        if dll.lower()!='kernel32.dll'or name not in['SetEnvironmentVariableA','GetEnvironmentVariableA']:
            return super().api(dll,name)
        argc=2 if name=='SetEnvironmentVariableA'else 3;a=self.args(argc)
        pc=self.environment_return_pc51();item=dict(name=name,args=a,return_PC=hex(pc),input_last_error=self.last_error,host_environment_accessed=False,host_environment_changed=False)
        try:
            env=self.ensure_environment51();key=self.cstring(a[0]);item['guest_name']=key.decode('ascii',errors='backslashreplace')
            if name=='SetEnvironmentVariableA':
                value=self.cstring(a[1])if a[1]else None
                item['guest_value']=value.decode('gbk',errors='backslashreplace')if value is not None else None
                if key.startswith(b'='):self.validate_hidden_environment_caller51(pc,a,key,value)
                result,error=env.set(key,value,self.last_error,self.guest_cwd51())
                item['guest_state_updated']=True
            else:
                result,error,body=env.get(key,a[2],self.last_error)
                if body is not None:
                    self.require_region(a[1],len(body),self.environment_write_permission51,'guest environment output')
                    self.uc.mem_write(a[1],body)
                item['bytes_written']=len(body)if body is not None else 0
            self.last_error=error;item.update(return_value=result,last_error=error)
            self.environment_calls51.append(item);self.api_trace.append(item);self.ret(result,argc)
        except PolicyError as e:self.environment_boundary51(item,str(e))
