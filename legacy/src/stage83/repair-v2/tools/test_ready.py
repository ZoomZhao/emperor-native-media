import io,json,sys,time
sys.dont_write_bytecode=True
from common import V,put,Boundary
from engine_ready import Memory
class OwnUC:
 def __init__(self):self.writes=[]
 def mem_write(self,a,b):self.writes.append((a,b))
u=OwnUC();m=Memory(u,[{'low':4096,'high':8191,'permissions':3}],io.BytesIO(),1);m.pc=123;m.write(4096,b'abc',15);assert u.writes==[(4096,b'abc')]and m.records==1
v=OwnUC();n=Memory(v,[{'low':4096,'high':8191,'permissions':3}],io.BytesIO(),1)
try:n.write(4096,b'abcde',15)
except Boundary:pass
else:raise AssertionError('Budget should reject before first own store')
assert v.writes==[]
# Original kind stops before importing/loading an engine, independent of config.
from engine_ready import run
try:run({'kind':'original'},'capture','NO_SOURCE',V/'runs')
except Boundary as e:assert 'CLOSED'in str(e)
else:raise AssertionError('Original gate open')
put(V/'evidence/ready-gate-tests.json',{'status':'PRE_STORE_PLATFORM_AND_ORIGINAL_CLOSED_TESTS_PASSED','cases':3,'Unicorn_or_original_execution':0,'three_byte_platform_chunk_supported':True,'whole_platform_write_reservation_precedes_any_store':True,'original_kind_rejected_before_engine_import':True});print('3 ready gate cases passed')
