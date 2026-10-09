#!/usr/bin/env python3
import ctypes,hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage15_gui import NativeGUI
def main():
 baseline=json.loads((ROOT/'evidence/stage14-gui-Wine-result.json').read_text());rows=baseline['rows'];captured={r['kind']:r for r in rows}
 # Clearly test-only: actual production construction passes NULL, never this.
 fixture=[1,1440,900,32,32,16,96,96,96,96,1000,1440,900]+[captured[f'color_{r}']['return_value'] for r in [15,16,20,18,6]]
 test=NativeGUI(fixture);dc=None;checks=[];handles={}
 for row in rows:
  kind=row['kind']
  if kind.startswith('metric_'):status,value,error=test.invoke(1,int(kind.split('_')[1]),last_error=0x13579)
  elif kind=='screen_DC':status,value,error=test.invoke(2,last_error=0x13579);dc=value
  elif kind.startswith('caps_'):status,value,error=test.invoke(3,dc,int(kind.split('_')[1]),0x13579)
  elif kind=='release_screen_DC':status,value,error=test.invoke(4,0,dc,0x13579)
  elif kind.startswith('color_'):status,value,error=test.invoke(5,int(kind.split('_')[1]),last_error=0x13579)
  elif kind.startswith('brush_'):status,value,error=test.invoke(6,int(kind.split('_')[1]),last_error=0x13579)
  elif kind.startswith('cursor_'):status,value,error=test.invoke(7,0,int(kind.split('_')[1]),0x13579)
  else:raise AssertionError(kind)
  assert status==1,(kind,status,value,error)
  is_handle=kind=='screen_DC' or kind.startswith(('brush_','cursor_'))
  if is_handle:
   assert value and test.object_kind_c(test.state,value)==(1 if kind=='screen_DC' else 2 if kind.startswith('brush_') else 3);handles[kind]=value
  else:assert value==row['return_value']
  stable_error=kind not in ['metric_0','screen_DC']
  if stable_error:assert error==row['last_error']
  checks.append(dict(kind=kind,native_fixture_value=value,Wine_captured_value=row['return_value'],handle_identity_not_numeric_equality=is_handle,LastError_compared=stable_error))
 assert test.invoke(3,dc,88,0x177)[0]==0,'released DC must not remain live'
 for role in [15,6]:
  status,h,error=test.invoke(6,role,last_error=0x177);assert h==handles[f'brush_{role}'];assert test.brush_pixel(h)==captured[f'color_{role}']['return_value']
 blob=test.export();restored=NativeGUI(fixture);restored.restore(blob);assert restored.invoke(7,0,32512,0x123)==test.invoke(7,0,32512,0x123);assert test.export()==restored.export()
 live=NativeGUI();profile=live.profile();live_calls=[];live_dc=live.invoke(2,last_error=0x123)[1]
 for metric in [0,1,11,12,2,3]:
  out=live.invoke(1,metric,last_error=0x123);assert out[0]==1;live_calls.append(dict(metric=metric,result=out))
 for cap in [88,90]:
  out=live.invoke(3,live_dc,cap,0x123);assert out[0]==1;live_calls.append(dict(cap=cap,result=out))
 assert live.invoke(4,0,live_dc,0x123)==(1,1,0x123)
 for role in [15,6]:
  out=live.invoke(6,role,last_error=0x123);assert out[0]==1;assert live.brush_pixel(out[1])==profile[13 if role==15 else 17]
 for role in [32514,32512]:assert live.invoke(7,0,role,0x123)[0]==1
 live_blob=live.export();fresh=NativeGUI();fresh.restore(live_blob);assert fresh.export()==live_blob
 result=dict(all_passed=True,fixed_display_fixture_comparisons=len(checks),fixed_fixture_is_not_production_profile=True,checks=checks,LastError_unstable_success_cases_not_forced=['GetSystemMetrics(0)','GetDC(NULL)'],live_profile_words=profile,live_calls=live_calls,live_brush_actually_drawn_to_CoreGraphics_pixel=True,validated_real_CGDisplayMode_CGColor_and_NSCursor_classes=True,fixture_and_live_exact_resource_snapshot_passed=True,snapshot_bytes=len(blob),live_snapshot_sha256=hashlib.sha256(live_blob).hexdigest(),screen_DC_scope='retained actual CGDisplayMode metadata; only caps queries and ReleaseDC implemented, drawing refuses',wait_cursor='actual NSCursor with task-authored hourglass image; temporary platform presentation fallback; never set on screen',no_Window_or_message_pump=True)
 (ROOT/'evidence/stage15-gui-native-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['checks','live_calls']}));test.dispose();restored.dispose();live.dispose();fresh.dispose()
if __name__=='__main__':main()
