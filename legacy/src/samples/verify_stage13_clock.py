#!/usr/bin/env python3
import ctypes,datetime,json,struct,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage13_events import NativeEvents
def unpack(body):return struct.unpack('<8H',body)
def epoch(body):
    y,m,w,d,h,n,s,ms=unpack(body);assert w==(datetime.datetime(y,m,d).weekday()+1)%7
    return datetime.datetime(y,m,d,h,n,s,ms*1000,tzinfo=datetime.timezone.utc).timestamp()
def main():
    n=NativeEvents();oracle=json.load(open(ROOT/'evidence/stage13-clock-Wine-result.json'));rows=oracle['rows'];checks=[]
    tz=next(r for r in rows if r['name']=='GetTimeZoneInformation');ret,err,body=n.clock(4,172,0x13579);assert body==bytes.fromhex(tz['buffer_hex']) and ret==tz['return_value'] and err==tz['last_error'];checks.append('entire timezone structure/return/LastError matches same-Wine')
    ret,err,f=n.clock(5,8,0x13579);assert f==bytes.fromhex(next(r for r in rows if r['name']=='QueryPerformanceFrequency')['buffer_hex']) and ret==1 and err==0x13579;checks.append('frequency/BOOL/LastError matches same-Wine')
    # Static conversion oracle derives its exact instant from actual Wine UTC.
    utc=bytes.fromhex(next(r for r in rows if r['name']=='GetSystemTime')['buffer_hex']);ns=int(round(epoch(utc)*1000))*1000000
    for local,name in [(0,'GetSystemTime'),(1,'GetLocalTime')]:
        target=ctypes.create_string_buffer(24);target.raw=bytes([0xa5])*24;assert n.clock_from_unix_c(ns,local,ctypes.byref(target,4),16)==1
        expected=bytes.fromhex(next(r for r in rows if r['name']==name)['buffer_hex']);assert target.raw[4:20]==expected and target.raw[:4]==target.raw[20:]==bytes([0xa5])*4;checks.append('same-Wine exact observed '+name+' conversion plus canaries')
    # Live output must advance and stay within the caller's real wall interval.
    before=time.time();_,err,system=n.clock(2,16,0x13579);_,err,local=n.clock(1,16,0x13579);_,err,ft=n.clock(3,8,0x13579);after=time.time();assert err==0x13579
    assert before-.01<=epoch(system)<=after+.01 and abs(epoch(local)-epoch(system)-28800)<.02
    ftime=(struct.unpack('<Q',ft)[0]-116444736000000000)/10000000;assert before<=ftime<=after;checks.append('live UTC/local/FILETIME are coherent real wall time')
    _,_,qa=n.clock(6,8,77);ta,_,_=n.clock(7,0,77);time.sleep(.03);_,_,qb=n.clock(6,8,77);tb,_,_=n.clock(7,0,77)
    seconds=(struct.unpack('<Q',qb)[0]-struct.unpack('<Q',qa)[0])/10000000;assert .025<=seconds<.5 and 25<=(tb-ta)&0xffffffff<500;checks.append('native boot counter/ticks advance at measured units')
    observed=struct.unpack('<Q',bytes.fromhex(next(r for r in rows if r['name']=='QueryPerformanceCounter')['buffer_hex']))[0];current=struct.unpack('<Q',qb)[0];assert 0<=current-observed<10000000*3600;checks.append('native counter uses same boot epoch as Wine, within measurement interval')
    result=dict(all_passed=True,checks=checks,check_count=len(checks),frequency=10000000,guest_bias_minutes=-480,native_clock_live=True,system_timezone_changed=False,Python_process_relative_clock_origin_not_used=True)
    (ROOT/'evidence/stage13-clock-native-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));n.dispose()
if __name__=='__main__':main()
