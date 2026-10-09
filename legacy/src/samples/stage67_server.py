"""Runs only inside native UI entry; explicit own adapter RPC, no guest imports."""
import base64,ctypes,hashlib,json,os,sys,threading,traceback
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage66_context_test import own_context
from stage43_window import NativeWindow
from stage67_dd import NativeDD
from stage45_pump import NativePump
from stage43_cursor import NativeCursorSelection
from stage43_input import NativeInput
from stage33_text import NativeText
objects={};kinds={};next_id=1
def pack(v):
 if isinstance(v,bytes):return {'bytes':base64.b64encode(v).decode()}
 if isinstance(v,(tuple,list)):return [pack(x) for x in v]
 if isinstance(v,dict):return {k:pack(x) for k,x in v.items()}
 return v
def unpack(v):
 if isinstance(v,dict) and 'bytes' in v:return base64.b64decode(v['bytes'],validate=True)
 if isinstance(v,list):return [unpack(x) for x in v]
 if isinstance(v,dict):return {k:unpack(x) for k,x in v.items()}
 return v
def get(i):
 if i not in objects:raise ValueError('unknown or disposed proxy handle')
 return objects[i]
def new(kind,args):
 global next_id
 if kind=='window':obj=NativeWindow()
 elif kind=='dd':
  window=get(args[0]);pointer=window.fn['pointer'](window.state,0x58000001)
  if not pointer:raise ValueError('owned window not restored')
  obj=NativeDD(pointer,args[1],args[2])
 elif kind=='pump':obj=NativePump(get(args[0]))
 elif kind=='cursor':obj=NativeCursorSelection(get(args[0]))
 elif kind=='input':obj=NativeInput(get(args[0]))
 elif kind=='text':obj=NativeText()
 else:raise ValueError('unsupported own adapter family')
 i=next_id;next_id+=1;objects[i]=obj;kinds[i]=kind;return i
def dispose(i):
 obj=get(i)
 if hasattr(obj,'dispose'):obj.dispose()
 del objects[i];del kinds[i]
def dispatch(q):
 op=q['op']
 if op=='HELLO':
  d=own_context();d.update(ppid=os.getppid(),native_UI_entry=True,AppKit_initialized_in_helper=True,Python_main_thread=threading.current_thread() is threading.main_thread(),
      no_guest_modules=not any(x.startswith('unicorn') for x in sys.modules));return d
 if op=='NEW':return new(q['kind'],q.get('args',[]))
 if op=='DISPOSE':dispose(q['target']);return True
 if op=='METHOD':
  obj=get(q['target']);name=q['name']
  if name not in ['export','restore','life_export','life_restore','query','geometry_report','diagnostic','resource','register','find','begin','dc','default',
      'display','info','pixels','storage_report','snapshot','route_snapshot','route_restore','peek','route_diagnostic','select','measure','draw']:
   raise ValueError('method not in bounded adapter protocol')
  args=unpack(q.get('args',[]))
  if kinds[q['target']]=='pump' and name=='peek' and (len(args)!=3 or tuple(args[1:]) not in [(0x200,0x209),(0x205,0x205)]):
   raise ValueError('only reviewed mouse-only filters allowed; no physical keyboard query')
  return pack(getattr(obj,name)(*args))
 if op=='FN':
  obj=get(q['target']);name=q['name']
  if name in ['new','delete','alt_state','window_activate','window_focus'] or name not in obj.fn:raise ValueError('unsafe or unknown native function')
  if name=='present' and q['args'][-1] is not None:raise ValueError('prototype has no PNG or screenshot output')
  if name=='pointer':
   if kinds[q['target']]!='window' or q['args'][1]!=0x58000001:raise ValueError('unowned pointer request')
   return dict(return_value={'remote_window':q['target']},buffers={})
  f=obj.fn[name];holders=[];native=[];buffers={}
  for n,a in enumerate(q['args']):
   if isinstance(a,dict) and 'state' in a:
    if a['state']!=q['target']:raise ValueError('cross-object state rejected')
    native.append(obj.state)
   elif isinstance(a,dict) and 'buffer' in a:
    raw=base64.b64decode(a['buffer'],validate=True)
    if len(raw)>32*1024*1024:raise ValueError('buffer above prototype bound')
    holder=ctypes.create_string_buffer(raw,len(raw));holders.append(holder);buffers[n]=holder
    typ=f.argtypes[n];native.append(holder if typ is ctypes.c_void_p else ctypes.cast(holder,typ))
   elif isinstance(a,dict) and 'cstring' in a:native.append(base64.b64decode(a['cstring'],validate=True))
   elif a is None:native.append(None)
   elif isinstance(a,(int,float)):native.append(a)
   else:raise ValueError('noncanonical argument')
  value=f(*native)
  return dict(return_value=value,buffers={str(n):base64.b64encode(b.raw).decode() for n,b in buffers.items()})
 if op=='SCENE_EXPORT':
  # One synchronous command on the helper main thread freezes all owned blobs.
  ids=q['targets'];scene=[]
  for i in ids:
   obj=get(i);kind=kinds[i];r=dict(id=i,kind=kind)
   if kind=='window':r.update(blob=obj.export(),life=obj.life_export())
   elif kind=='dd':r.update(blob=obj.export())
   elif kind=='pump':r.update(blob=obj.snapshot(),route=obj.route_snapshot())
   elif kind=='cursor':r.update(blob=obj.snapshot())
   else:raise ValueError('non-state family in snapshot')
   scene.append(r)
  result=pack(dict(version=1,objects=scene));body=json.dumps(result,sort_keys=True,separators=(',',':')).encode()
  return dict(state=result,sha256=hashlib.sha256(body).hexdigest())
 if op=='CLOSE':return True
 raise ValueError('unsupported protocol operation')
try:
 for line in sys.stdin:
  q=json.loads(line)
  try:body=dict(ok=True,result=dispatch(q))
  except Exception as e:body=dict(ok=False,error=type(e).__name__,message=str(e))
  print(json.dumps(dict(id=q['id'],**body),separators=(',',':')),flush=True)
  if q.get('op')=='CLOSE':break
finally:
 for i in list(objects)[::-1]:
  try:dispose(i)
  except Exception:traceback.print_exc(file=sys.stderr)
