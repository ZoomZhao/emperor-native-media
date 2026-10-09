"""Observe all root records; never writes or drops a receiver from execution."""
import json,struct
from pathlib import Path
from common import Boundary,put
def inspect(snapshot):
 p=Path(snapshot);s=json.loads(p.read_text());f=(p.parent/'memory.bin').open('rb')
 def read(a,n):
  for r in s['regions']:
   if r['low']<=a and a+n<=r['high']+1:f.seek(r['offset']+a-r['low']);b=f.read(n);assert len(b)==n;return b
  raise Boundary('ACTIVITY_READ_UNMAPPED '+hex(a))
 def word(a):return struct.unpack('<I',read(a,4))[0]
 try:
  begin,end=word(0x10bc918),word(0x10bc91c);objects,objects_end=word(0x8c7638),word(0x8c763c)
  if end<begin or (end-begin)%4 or end-begin>400000 or objects_end<objects or (objects_end-objects)%4 or objects_end-objects>1048576:raise Boundary('ACTIVITY_VECTOR_INVALID')
  rows=[];eligible=0
  for i in range(1,(end-begin)//4):
   record=word(begin+4*i);row={'index':i,'record':record}
   if record:
    active=read(record+12,1)[0];index=struct.unpack('<h',read(record+8,2))[0];row.update(record_active=active,object_index=index)
    if active and index:
     # No artificial range filter: observe the original uint32 indexing.
     obj=word((objects+index*4)&0xffffffff);state=read(obj+4,1)[0];row.update(object=obj,object_state=state,vtable=word(obj));eligible+=int(state in (1,3))
   rows.append(row)
  return {'status':'ACTIVITY_OBSERVED_ONLY','vector_records':(end-begin)//4,'eligible_receivers':eligible,'records':rows,'input_or_execution_filter_added':False,'gameplay_operation_for_nonempty_input':'需要由真实游戏模拟自然产生向量中激活且索引非零、对象状态为1或3的更新对象；具体建筑/单位类型尚未由现有证据映射，不能承诺放某一种建筑即可。可提供正常游戏操作后该活动路径的真实检查点；不修改存档或活动字节。'}
 except Exception as e:return {'status':'ACTIVITY_UNDETERMINED','error':str(e),'input_or_execution_filter_added':False}
 finally:f.close()
