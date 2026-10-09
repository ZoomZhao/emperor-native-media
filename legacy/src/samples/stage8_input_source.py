#!/usr/bin/env python3
"""Select an already enabled input source, with recorded restoration ID."""
import ctypes as c, json, sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
hit=c.CDLL('/System/Library/Frameworks/Carbon.framework/Frameworks/HIToolbox.framework/HIToolbox')
cf=c.CDLL('/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation')
hit.TISCopyCurrentKeyboardInputSource.restype=c.c_void_p
hit.TISGetInputSourceProperty.argtypes=[c.c_void_p,c.c_void_p];hit.TISGetInputSourceProperty.restype=c.c_void_p
hit.TISCreateInputSourceList.argtypes=[c.c_void_p,c.c_bool];hit.TISCreateInputSourceList.restype=c.c_void_p
hit.TISSelectInputSource.argtypes=[c.c_void_p];hit.TISSelectInputSource.restype=c.c_int32
cf.CFArrayGetCount.argtypes=[c.c_void_p];cf.CFArrayGetCount.restype=c.c_long
cf.CFArrayGetValueAtIndex.argtypes=[c.c_void_p,c.c_long];cf.CFArrayGetValueAtIndex.restype=c.c_void_p
cf.CFStringGetCString.argtypes=[c.c_void_p,c.c_char_p,c.c_long,c.c_uint32];cf.CFStringGetCString.restype=c.c_bool
cf.CFRelease.argtypes=[c.c_void_p]
prop=c.c_void_p.in_dll(hit,'kTISPropertyInputSourceID').value
def identifier(source):
    v=hit.TISGetInputSourceProperty(source,prop);buffer=c.create_string_buffer(1024)
    assert v and cf.CFStringGetCString(v,buffer,1024,0x08000100)
    return buffer.value.decode('utf-8')
current=hit.TISCopyCurrentKeyboardInputSource();before=identifier(current);cf.CFRelease(current)
available=hit.TISCreateInputSourceList(None,False)
sources={identifier(cf.CFArrayGetValueAtIndex(available,i)):cf.CFArrayGetValueAtIndex(available,i) for i in range(cf.CFArrayGetCount(available))}
action=sys.argv[1] if len(sys.argv)>1 else 'list'
prefix=sys.argv[2] if len(sys.argv)>2 else 'stage8'
assert prefix in ['stage8','stage9']
result=dict(action=action,before=before,enabled_sources=list(sources),installed_or_removed_sources=False)
if action=='ascii':
    target=next((name for name in ['com.apple.keylayout.ABC','com.apple.keylayout.US'] if name in sources),None)
    assert target,'no existing enabled ABC/US input source'
    record=root/f'evidence/{prefix}-input-source-original.json'
    if not record.exists():record.write_text(json.dumps(dict(original_input_source=before),indent=2)+'\n')
elif action=='restore':target=json.loads((root/f'evidence/{prefix}-input-source-original.json').read_text())['original_input_source']
else:assert action=='list';target=None
if target:
    assert target in sources,'recorded input source no longer enabled'
    result.update(selected=target,select_status=hit.TISSelectInputSource(sources[target]))
    assert result['select_status']==0,result
    current=hit.TISCopyCurrentKeyboardInputSource();result['after']=identifier(current);cf.CFRelease(current)
    assert result['after']==target
cf.CFRelease(available)
(root/f'evidence/{prefix}-input-source-{action}.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
