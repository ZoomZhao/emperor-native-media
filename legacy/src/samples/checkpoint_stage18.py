#!/usr/bin/env python3
"""One exact legal campaign dependency; no arbitrary guest filesystem access."""
import argparse,hashlib,json,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage17 import FileRuntime,x,u
from stage17_file import NativeSavedFile
from stage7_runtime import Boundary,GUEST_NAME
from stage12_state import save_state,restore_state
CAMPAIGN=b'C:\\Emperor\\Campaigns\\1 Xia Dynasty - Tutorials.pak'
RELATIVE=b'Campaigns\\1 Xia Dynasty - Tutorials.pak'
class CampaignRuntime(FileRuntime):
 extra_snapshot_fields=FileRuntime.extra_snapshot_fields+['campaign_identity','campaign_IO','campaign_calls']
 def __init__(self,save):
  super().__init__(save);record=json.loads((ROOT/'evidence/stage17-campaign-dependency.json').read_text());copy=Path(record['copy_path']);source=Path(record['original_path']);assert copy.resolve().is_relative_to(ROOT);self.campaign_payload=copy.read_bytes();assert len(self.campaign_payload)==record['copy_size']==186403 and hashlib.sha256(self.campaign_payload).hexdigest()==record['copy_sha256']==record['original_sha256'];assert source.read_bytes()==self.campaign_payload
  self.native_campaign=NativeSavedFile(copy);info=self.native_campaign.info();self._campaign_identity=(info[0],info[4],info[5],record['copy_sha256']);assert self.native_campaign.read(0,len(self.campaign_payload))==self.campaign_payload;self.campaign_IO=[];self.campaign_calls=[]
 @property
 def campaign_identity(self):return self._campaign_identity
 @campaign_identity.setter
 def campaign_identity(self,value):assert tuple(value)==self._campaign_identity
 def volume_consumer_paths(self):return super().volume_consumer_paths()|{CAMPAIGN.lower()}
 def native_for_handle(self,handle):
  f=self.files.get(handle,{});return self.native_campaign if f.get('path')==CAMPAIGN.lower() and f.get('payload')==self.campaign_payload else super().native_for_handle(handle)
 def native_for_path(self,path):return self.native_campaign if path.lower()==CAMPAIGN.lower() else super().native_for_path(path)
 def record_native_read(self,item,backing):
  if backing is self.native_campaign:self.campaign_IO.append(item)
  else:super().record_native_read(item,backing)
 def api(self,dll,name):
  if dll.lower()=='kernel32.dll' and name=='GetFullPathNameA' and self.cstring(self.args(4)[0]).lower() in {RELATIVE.lower(),CAMPAIGN.lower()}:
   a=self.args(4);path=self.cstring(a[0]);item=dict(name=name,args=[hex(v) for v in a],input=path.decode(),canonical=CAMPAIGN.decode(),backend='exact readonly input alias rooted in already verified native cwd C:\\Emperor',return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))));self.campaign_calls.append(item);self.api_trace.append(item)
   if a[1]<=len(CAMPAIGN):self.ret(len(CAMPAIGN)+1,4);return
   self.require_region(a[2],len(CAMPAIGN)+1,u.UC_PROT_WRITE,'campaign full path')
   if a[3]:self.require_region(a[3],4,u.UC_PROT_WRITE,'campaign basename pointer')
   self.uc.mem_write(a[2],CAMPAIGN+b'\0')
   if a[3]:self.word(a[3],a[2]+CAMPAIGN.rfind(b'\\')+1)
   self.ret(len(CAMPAIGN),4)
  elif dll.lower()=='kernel32.dll' and name=='CreateFileA' and self.cstring(self.args(7)[0]).lower() in {RELATIVE.lower(),CAMPAIGN.lower()}:
   a=self.args(7);path=self.cstring(a[0]);item=dict(name=name,args=[hex(v) for v in a],path=path.decode(),return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),readonly_source_sha256=self._campaign_identity[3]);self.campaign_calls.append(item);self.api_trace.append(item)
   if a[1]!=0x80000000 or a[2]&~7 or a[4]!=3 or a[5]!=0x80 or a[6]:raise Boundary('campaign_open_access_flags_scope',**item)
   if a[3] and (self.readword(a[3])!=12 or self.readword(a[3]+4)!=0 or self.readword(a[3]+8) not in (0,1)):raise Boundary('campaign_security_scope',**item)
   if any(f.get('path')==CAMPAIGN.lower() for f in self.files.values()):raise Boundary('campaign_multiple_handle_sharing_scope',**item)
   handle=self.next_handle;self.next_handle+=1;self.files[handle]=dict(path=CAMPAIGN.lower(),payload=self.campaign_payload,position=0);item['return_value']=hex(handle);self.ret(handle,7)
  else:super().api(dll,name)
 def dispose(self):self.native_campaign.dispose();self.native_save.dispose();self.paths.dispose();self.gui.dispose();self.messages.dispose();self.events.dispose()
ELEVATION=b'C:\\Emperor\\data\\China_Elevation.sg3'
ELEVATION_REL=b'data\\China_Elevation.sg3'
class ElevationRuntime(CampaignRuntime):
 extra_snapshot_fields=CampaignRuntime.extra_snapshot_fields+['elevation_identity','elevation_IO','elevation_calls']
 def __init__(self,save):
  super().__init__(save);m=json.loads((ROOT/'evidence/stage18-elevation-dependency.json').read_text());copy=Path(m['copy_path']);assert copy.resolve().is_relative_to(ROOT);self.elevation_payload=copy.read_bytes();assert len(self.elevation_payload)==m['size']==695080 and hashlib.sha256(self.elevation_payload).hexdigest()==m['sha256'];assert Path(m['original_path']).read_bytes()==self.elevation_payload
  self.native_elevation=NativeSavedFile(copy);info=self.native_elevation.info();self._elevation_identity=(info[0],info[4],info[5],m['sha256']);assert self.native_elevation.read(0,len(self.elevation_payload))==self.elevation_payload;self.elevation_IO=[];self.elevation_calls=[]
 @property
 def elevation_identity(self):return self._elevation_identity
 @elevation_identity.setter
 def elevation_identity(self,value):assert tuple(value)==self._elevation_identity
 def volume_consumer_paths(self):return super().volume_consumer_paths()|{ELEVATION.lower()}
 def native_for_handle(self,handle):
  f=self.files.get(handle,{});return self.native_elevation if f.get('path')==ELEVATION.lower() and f.get('payload')==self.elevation_payload else super().native_for_handle(handle)
 def native_for_path(self,path):return self.native_elevation if path.lower() in {ELEVATION.lower(),ELEVATION_REL.lower()} else super().native_for_path(path)
 def record_native_read(self,item,backing):
  if backing is self.native_elevation:self.elevation_IO.append(item)
  else:super().record_native_read(item,backing)
 def api(self,dll,name):
  if dll.lower()=='kernel32.dll' and name=='CreateFileA' and self.cstring(self.args(7)[0]).lower() in {ELEVATION.lower(),ELEVATION_REL.lower()}:
   a=self.args(7);item=dict(name=name,args=[hex(v) for v in a],path=self.cstring(a[0]).decode(),return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),readonly_source_sha256=self._elevation_identity[3]);self.elevation_calls.append(item);self.api_trace.append(item)
   if a[1]!=0x80000000 or a[2]!=3 or a[4]!=3 or a[5]!=0x80 or a[6]:raise Boundary('elevation_open_access_flags_scope',**item)
   if a[3] and (self.readword(a[3])!=12 or self.readword(a[3]+4)!=0 or self.readword(a[3]+8) not in (0,1)):raise Boundary('elevation_security_scope',**item)
   if any(f.get('path')==ELEVATION.lower() for f in self.files.values()):raise Boundary('elevation_multiple_handle_sharing_scope',**item)
   handle=self.next_handle;self.next_handle+=1;self.files[handle]=dict(path=ELEVATION.lower(),payload=self.elevation_payload,position=0);item['return_value']=hex(handle);self.ret(handle,7)
  else:super().api(dll,name)
 def dispose(self):self.native_elevation.dispose();super().dispose()
PIXELS=b'C:\\Emperor\\data\\China_Elevation.555'
PIXELS_REL=b'data\\China_Elevation.555'
class PixelsRuntime(ElevationRuntime):
 extra_snapshot_fields=ElevationRuntime.extra_snapshot_fields+['pixels_identity','pixels_IO','pixels_calls']
 def __init__(self,save):
  super().__init__(save);m=json.loads((ROOT/'evidence/stage18-pixels-dependency.json').read_text());copy=Path(m['copy_path']);assert copy.resolve().is_relative_to(ROOT);self.pixels_payload=copy.read_bytes();assert len(self.pixels_payload)==m['size']==2067457 and hashlib.sha256(self.pixels_payload).hexdigest()==m['sha256'];assert Path(m['original_path']).read_bytes()==self.pixels_payload
  self.native_pixels=NativeSavedFile(copy);info=self.native_pixels.info();self._pixels_identity=(info[0],info[4],info[5],m['sha256']);assert self.native_pixels.read(0,len(self.pixels_payload))==self.pixels_payload;self.pixels_IO=[];self.pixels_calls=[]
 @property
 def pixels_identity(self):return self._pixels_identity
 @pixels_identity.setter
 def pixels_identity(self,value):assert tuple(value)==self._pixels_identity
 def volume_consumer_paths(self):return super().volume_consumer_paths()|{PIXELS.lower()}
 def native_for_handle(self,handle):
  f=self.files.get(handle,{});return self.native_pixels if f.get('path')==PIXELS.lower() and f.get('payload')==self.pixels_payload else super().native_for_handle(handle)
 def native_for_path(self,path):return self.native_pixels if path.lower() in {PIXELS.lower(),PIXELS_REL.lower()} else super().native_for_path(path)
 def record_native_read(self,item,backing):
  if backing is self.native_pixels:self.pixels_IO.append(item)
  else:super().record_native_read(item,backing)
 def api(self,dll,name):
  if dll.lower()=='kernel32.dll' and name=='CreateFileA' and self.cstring(self.args(7)[0]).lower() in {PIXELS.lower(),PIXELS_REL.lower()}:
   a=self.args(7);item=dict(name=name,args=[hex(v) for v in a],path=self.cstring(a[0]).decode(),return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),readonly_source_sha256=self._pixels_identity[3]);self.pixels_calls.append(item);self.api_trace.append(item)
   if a[1]!=0x80000000 or a[2]!=3 or a[4]!=3 or a[5]!=0x80 or a[6]:raise Boundary('pixels_open_access_flags_scope',**item)
   if a[3] and (self.readword(a[3])!=12 or self.readword(a[3]+4)!=0 or self.readword(a[3]+8) not in (0,1)):raise Boundary('pixels_security_scope',**item)
   if any(f.get('path')==PIXELS.lower() for f in self.files.values()):raise Boundary('pixels_multiple_handle_sharing_scope',**item)
   handle=self.next_handle;self.next_handle+=1;self.files[handle]=dict(path=PIXELS.lower(),payload=self.pixels_payload,position=0);item['return_value']=hex(handle);self.ret(handle,7)
  else:super().api(dll,name)
 def dispose(self):self.native_pixels.dispose();super().dispose()
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume-state',default='classes-return');p.add_argument('--tag',default='campaign');p.add_argument('--elevation',action='store_true');p.add_argument('--pixels',action='store_true');a=p.parse_args();started=time.monotonic();r=(PixelsRuntime if a.pixels else ElevationRuntime if a.elevation else CampaignRuntime)('ST9BASE');previous=restore_state(r,a.resume_state);before=len(r.api_trace)
 if r.first_save_read is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 s=r.segment()
 while s['status']=='budget_yield' and time.monotonic()-started<40:s=r.segment()
 save_state(r,a.tag,previous['phase'],previous['milestones']+[dict(name=previous['phase'],result=s)])
 result=dict(elapsed_seconds=round(time.monotonic()-started,3),source_checkpoint=a.resume_state,final_status=s['status'],final_boundary=s.get('boundary'),final_registers=s['registers'],campaign_calls=r.campaign_calls,campaign_IO=r.campaign_IO,campaign_read_bytes=sum(i.get('bytes',0) for i in r.campaign_IO),save_read_bytes=sum(i.get('bytes',0) for i in r.save_IO),path_events=r.path_events,city_deserializer_returned=any(e['label']=='city_deserializer_returned' for e in r.path_events),postload_finalizer_returned=any(e['label']=='city_root_postload_finalizer_returned' for e in r.path_events),original_city_root_returned=s['status']=='returned',original_root_return_value=s.get('eax'),original_startup_path_field=r.cstring(0x10dd7f4).decode('ascii',errors='backslashreplace'),natural_gate=r.readword(0x88ec38),natural_phase=r.readword(0xc82ef8),new_API_calls=r.api_trace[before:],full_playable_city_proven=False,native_city_passed=False,original_code_not_patched=True,state_directory=str(ROOT/'runs'/('stage12-state-'+a.tag)),backend='original i386 reference via approved Unicorn and own native platform helpers')
 if a.elevation or a.pixels:result.update(elevation_IO=r.elevation_IO,elevation_calls=r.elevation_calls,elevation_read_bytes=sum(i.get('bytes',0) for i in r.elevation_IO))
 if a.pixels:result.update(pixels_IO=r.pixels_IO,pixels_calls=r.pixels_calls,pixels_read_bytes=sum(i.get('bytes',0) for i in r.pixels_IO))
 (ROOT/f'evidence/stage18-checkpoint-{a.tag}.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['new_API_calls','path_events','campaign_IO','campaign_calls','elevation_IO','elevation_calls','pixels_IO','pixels_calls']}));r.dispose()
if __name__=='__main__':main()
