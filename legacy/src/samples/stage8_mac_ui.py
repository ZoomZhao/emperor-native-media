#!/usr/bin/env python3
"""Local Apple API UI helper; only interacts with inspected game-owned windows."""
import argparse, ctypes as c, json, subprocess, time
from pathlib import Path
root=Path(__file__).resolve().parents[1]
cg=c.CDLL('/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics')
cf=c.CDLL('/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation')
cf.CFStringCreateWithCString.argtypes=[c.c_void_p,c.c_char_p,c.c_uint32];cf.CFStringCreateWithCString.restype=c.c_void_p
cf.CFArrayGetCount.argtypes=[c.c_void_p];cf.CFArrayGetCount.restype=c.c_long
cf.CFArrayGetValueAtIndex.argtypes=[c.c_void_p,c.c_long];cf.CFArrayGetValueAtIndex.restype=c.c_void_p
cf.CFDictionaryGetValue.argtypes=[c.c_void_p,c.c_void_p];cf.CFDictionaryGetValue.restype=c.c_void_p
cf.CFStringGetCString.argtypes=[c.c_void_p,c.c_char_p,c.c_long,c.c_uint32];cf.CFStringGetCString.restype=c.c_bool
cf.CFNumberGetValue.argtypes=[c.c_void_p,c.c_int,c.c_void_p];cf.CFNumberGetValue.restype=c.c_bool
cf.CFRelease.argtypes=[c.c_void_p]
cg.CGWindowListCopyWindowInfo.argtypes=[c.c_uint32,c.c_uint32];cg.CGWindowListCopyWindowInfo.restype=c.c_void_p
cg.CGPreflightScreenCaptureAccess.restype=c.c_bool;cg.CGPreflightPostEventAccess.restype=c.c_bool
def key(name):return cf.CFStringCreateWithCString(None,name.encode(),0x08000100)
def lookup(d,name):
    k=key(name);v=cf.CFDictionaryGetValue(d,k);cf.CFRelease(k);return v
def string(v):
    if not v:return ''
    b=c.create_string_buffer(4096)
    return b.value.decode('utf-8') if cf.CFStringGetCString(v,b,4096,0x08000100) else ''
def number(v):
    n=c.c_double()
    return float(n.value) if v and cf.CFNumberGetValue(v,13,c.byref(n)) else 0
def windows():
    # Include windows hidden by focus/Space changes; GUI games can hide their
    # drawing surface when ChatGPT becomes frontmost to display a screenshot.
    result=[];a=cg.CGWindowListCopyWindowInfo(16,0)
    if not a:return result
    for i in range(cf.CFArrayGetCount(a)):
        d=cf.CFArrayGetValueAtIndex(a,i);owner=string(lookup(d,'kCGWindowOwnerName'));title=string(lookup(d,'kCGWindowName'))
        if not any(s in (owner+' '+title).lower() for s in ['wine','emperor','皇帝','xquartz','coreservicesuiagent']):continue
        bounds=lookup(d,'kCGWindowBounds')
        result.append(dict(id=int(number(lookup(d,'kCGWindowNumber'))),pid=int(number(lookup(d,'kCGWindowOwnerPID'))),owner=owner,title=title,
                           layer=int(number(lookup(d,'kCGWindowLayer'))),bounds={k:number(lookup(bounds,k)) for k in ['X','Y','Width','Height']}))
    cf.CFRelease(a);return result
def game_window(window_id):
    found=next((w for w in windows() if w['id']==window_id),None)
    assert found and 'coreservicesuiagent' not in found['owner'].lower(), 'no inspected game window at this ID'
    assert found['bounds']['Width']>100 and found['bounds']['Height']>100
    return found
class Point(c.Structure):_fields_=[('x',c.c_double),('y',c.c_double)]
cg.CGEventCreateMouseEvent.argtypes=[c.c_void_p,c.c_uint32,Point,c.c_uint32];cg.CGEventCreateMouseEvent.restype=c.c_void_p
cg.CGEventCreateKeyboardEvent.argtypes=[c.c_void_p,c.c_uint16,c.c_bool];cg.CGEventCreateKeyboardEvent.restype=c.c_void_p
cg.CGEventKeyboardSetUnicodeString.argtypes=[c.c_void_p,c.c_ulong,c.POINTER(c.c_uint16)]
cg.CGEventSetFlags.argtypes=[c.c_void_p,c.c_uint64]
cg.CGEventPost.argtypes=[c.c_uint32,c.c_void_p]
def focus(w):
    # Cocoa activation for the known window owner, without changing user settings.
    objc=c.CDLL('/usr/lib/libobjc.A.dylib');c.CDLL('/System/Library/Frameworks/AppKit.framework/AppKit')
    objc.objc_getClass.argtypes=[c.c_char_p];objc.objc_getClass.restype=c.c_void_p
    objc.sel_registerName.argtypes=[c.c_char_p];objc.sel_registerName.restype=c.c_void_p
    address=c.cast(objc.objc_msgSend,c.c_void_p).value
    get=c.CFUNCTYPE(c.c_void_p,c.c_void_p,c.c_void_p,c.c_int)(address)
    activate=c.CFUNCTYPE(c.c_bool,c.c_void_p,c.c_void_p,c.c_ulong)(address)
    app=get(objc.objc_getClass(b'NSRunningApplication'),objc.sel_registerName(b'runningApplicationWithProcessIdentifier:'),w['pid'])
    if app:activate(app,objc.sel_registerName(b'activateWithOptions:'),2)
    time.sleep(.2)
def click(w,x,y):
    b=w['bounds'];assert 0<=x<b['Width'] and 0<=y<b['Height']
    p=Point(b['X']+x,b['Y']+y)
    for kind in [5,1,2]:
        e=cg.CGEventCreateMouseEvent(None,kind,p,0);cg.CGEventPost(0,e);cf.CFRelease(e);time.sleep(.07)
def move(w,x,y):
    b=w['bounds'];assert 0<=x<b['Width'] and 0<=y<b['Height']
    e=cg.CGEventCreateMouseEvent(None,5,Point(b['X']+x,b['Y']+y),0)
    cg.CGEventPost(0,e);cf.CFRelease(e);time.sleep(.2)
def press(code):
    for down in [True,False]:
        e=cg.CGEventCreateKeyboardEvent(None,code,down);cg.CGEventPost(0,e);cf.CFRelease(e);time.sleep(.05)
def type_text(text):
    assert text.isascii() and len(text)<=64
    # Pair real Apple ANSI keycodes with their text and input-poll intervals;
    # no input-source or OS setting is changed.
    codes={'a':0,'s':1,'d':2,'f':3,'h':4,'g':5,'z':6,'x':7,'c':8,'v':9,'b':11,'q':12,'w':13,'e':14,'r':15,'y':16,'t':17,'1':18,'2':19,'3':20,'4':21,'6':22,'5':23,'=':24,'9':25,'7':26,'-':27,'8':28,'0':29,'o':31,'u':32,'i':34,'p':35,'l':37,'j':38,'k':40,'n':45,'m':46}
    for ch in text:
        assert ch.lower() in codes
        for down in [True,False]:
            e=cg.CGEventCreateKeyboardEvent(None,codes[ch.lower()],down)
            chars=(c.c_uint16*1)(ord(ch))
            cg.CGEventKeyboardSetUnicodeString(e,1,chars)
            cg.CGEventSetFlags(e,(1<<17) if ch.isupper() else 0)
            cg.CGEventPost(0,e);cf.CFRelease(e)
            # Leave a complete game input-poll interval between down and up.
            time.sleep(.07)
        time.sleep(.04)
p=argparse.ArgumentParser();p.add_argument('action',choices=['list','capture','click','move','key','type','loadfreeze']);p.add_argument('--window',type=int);p.add_argument('--label',default='current');p.add_argument('--x',type=float);p.add_argument('--y',type=float);p.add_argument('--key',type=int);p.add_argument('--text');p.add_argument('--clear',action='store_true')
a=p.parse_args();assert a.label.replace('-','').isalnum()
if a.action=='list':print(json.dumps(windows(),ensure_ascii=False,indent=2))
else:
    w=game_window(a.window)
    if a.action in ['capture','loadfreeze']:
        assert cg.CGPreflightScreenCaptureAccess(),'screen capture not approved in this execution context'
        if a.action=='loadfreeze':
            assert cg.CGPreflightPostEventAccess(),'event posting not approved in this execution context'
            focus(w)
            click(w,650,547)
            time.sleep(.25)
            game_window(w['id'])
            click(w,23,53)
        image=root/f'evidence/stage8-{a.label}.png'
        r=subprocess.run(['/usr/sbin/screencapture','-x','-o','-l',str(w['id']),str(image)],capture_output=True,text=True,timeout=10)
        assert r.returncode==0 and image.exists(),r.stderr
        result=dict(action=a.action,window=w,path=str(image))
        if a.action=='loadfreeze':result.update(load_click=[650,547],file_menu_click=[23,53],between_click_delay_seconds=.25)
    else:
        assert cg.CGPreflightPostEventAccess(),'event posting not approved in this execution context'
        focus(w)
        if a.action=='click':click(w,a.x,a.y)
        elif a.action=='move':move(w,a.x,a.y)
        elif a.action=='key':assert 0<=a.key<=127;press(a.key)
        else:
            if a.clear:
                for _ in range(32):press(51)
            type_text(a.text)
        result=dict(action=a.action,window=w,x=a.x,y=a.y,key=a.key,text=a.text)
    (root/f'evidence/stage8-ui-{a.label}.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');print(json.dumps(result,ensure_ascii=False))
