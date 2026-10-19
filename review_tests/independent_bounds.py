"""Independent authored building-profile boundary checks; no asset reads."""
from pathlib import Path
import argparse,importlib.util,json,struct,sys
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
sys.path.insert(0,str(args.source_root/'scripts'))
import building_record_audit as a
checks=0;groups={}
def need(v,label):
    global checks
    checks+=1;groups[label]=groups.get(label,0)+1
    if not v:raise AssertionError(label)
def u16(v):return v.to_bytes(2,'little')
def named(name):
    raw=name.encode('ascii');return u16(65535)+u16(0)+u16(len(raw))+raw
def own_common(version,kind,x=7,y=9,n=2,alive=0xa5):
    # Sequential schema construction, no audit offsets/span dictionaries.
    b=u16(version)+bytes([alive])+bytes(2)+bytes([n])+bytes(2)
    b+=u16(x)+u16(y)+bytes(4)+u16(kind)+bytes(94)
    if version==5:b+=b'\x43'
    b+=b'\x6a'+bytes(66 if version==3 else 68)
    return b
def own_record(version,kind,tail,name=None,**fields):
    return (named(name)if name is not None else u16(0x8001))+own_common(version,kind,**fields)+bytes(tail)
def preamble():return u16(1)+(4000).to_bytes(4,'little')
for v in (3,4,5):
    root=own_record(v,0,0,'Building',n=0,alive=0)
    well=own_record(v,72,130,'cWellBldg',x=65535,y=32768)
    unknown=own_record(v,0x7c,0,'cInspectorBldg')
    b=b'odd'+preamble()+root+well+unknown
    r=a.parse_table(b,3)
    need(r['status']=='unsupported_tail'and len(r['records'])==2,'closed_prefix')
    w=r['records'][1]
    need((w['x'],w['y'],w['alive'],w['size'],w['type'],w['common_rotation'])==(65535,32768,0xa5,2,72,0x6a),'sequential_field_oracle')
    need(w['start']==3+6+len(root)and w['end']==3+6+len(root)+len(well),'span_oracle')
    for end in range(3,len(b)):
        r=a.parse_table(b,3,end)
        need(not r['complete']and r['records']==[],'every_truncation_clears_prefix')
    filler=own_record(v,0,0)
    full=preamble()+root+well+filler*3998+b'next-section'
    r=a.parse_table(full,0)
    need(r['complete']and r['end']==len(full)-12 and len(r['records'])==4000,'exact_count_and_next_section')
    need(a.parse_table(full[:-13],0)['records']==[],'last_slot_truncation')
prefix=preamble()+own_record(4,0,0,'Building')+own_record(4,72,130,'cWellBldg')
for tag in (0,1,9,10,0x7fff):
    b=prefix+u16(tag)+own_common(4,0)
    need(a.parse_table(b,0)['status']=='unsupported_tag'and a.parse_table(b,0)['records']==[],'low_tags_no_resync')
for version in (0,1,2,6,65535):
    b=prefix+u16(0x8001)+u16(version)+bytes(180)
    need(a.parse_table(b,0)['status']=='unsupported_version'and a.parse_table(b,0)['records']==[],'bad_version_clears')
for raw_header in (u16(65535)+u16(1)+u16(1)+b'x',u16(65535)+u16(0)+u16(1)+b'\x80',u16(65535)+u16(0)+u16(1)+b'\0',u16(65535)+u16(0)+u16(0),u16(65535)+u16(0)+u16(256)+b'x'*256):
    r=a.parse_table(prefix+raw_header+own_common(4,0),0)
    need(r['status']=='class_header'and r['records']==[],'malformed_names_clear')
for start,limit in ((-1,len(prefix)),(0,-1),(0,len(prefix)+1),(len(prefix)+1,len(prefix)),(1,0)):
    need(a.parse_table(prefix,start,limit)['records']==[],'explicit_bounds')
# Every footprint bit is counted independently; no on-map/flag conflation.
record={'type':72,'name':'cWellBldg','alive':1,'size':2,'x':11,'y':13}
side=112;origin=(228-side)//2;cells=[(13+origin+dy)*228+11+origin+dx for dy in range(2)for dx in range(2)]
for off in range(16):
    for building in range(16):
        terrain=[0x80000]*51984
        for i,at in enumerate(cells):terrain[at]=(0x80000 if off&(1<<i)else 0)|(8 if building&(1<<i)else 0)
        got=a.footprint_control(record,side,terrain)
        need(got['on_map']==4-bin(off).count('1')and got['building_flag']==bin(building).count('1')and got['cells']==4,'independent_footprint_bit_counts')
for change in ({'x':111},{'y':111},{'x':65535},{'y':65535},{'size':0},{'size':9},{'alive':0},{'name':'cOther'},{'type':0}):
    need(a.footprint_control({**record,**change},112,[8]*51984)is None,'nonplaceable_rejected')
result={'passed':True,'checks':checks,'groups':groups,'authored_only':True,'original_data_reads':0}
args.output.write_text(json.dumps(result,indent=2));print(json.dumps(result))
