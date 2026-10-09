import hashlib,json,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1]
base=R/'samples/stage31_window.m';body=base.read_text().replace('S31Window','S44Window').replace('S44Window:NSWindow','S44Window:NSPanel').replace('canBecomeKeyWindow{return YES;}','canBecomeKeyWindow{return NO;}').replace('canBecomeMainWindow{return YES;}','canBecomeMainWindow{return NO;}')
body=body.replace('setActivationPolicy:NSApplicationActivationPolicyRegular','setActivationPolicy:NSApplicationActivationPolicyProhibited')
body=body.replace('[NSApplication sharedApplication];Core*c=calloc','[NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];Core*c=calloc')
body=body.replace('*out=NSWindowStyleMaskBorderless;','*out=NSWindowStyleMaskBorderless|NSWindowStyleMaskNonactivatingPanel;').replace('NSWindowStyleMaskMiniaturizable;','NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskNonactivatingPanel;')
body=body.replace('win.backgroundColor=NSColor.blackColor;return win;','win.backgroundColor=NSColor.blackColor;[(NSPanel*)win setHidesOnDeactivate:NO];[(NSPanel*)win setFloatingPanel:NO];return win;')
body+='\nint s44_visible(Core*c,uint32_t*out){if(!c||!c->window||!out)return 0;[NSApp updateWindows];[c->window displayIfNeeded];[[NSRunLoop currentRunLoop]runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.002]];if(NSRunningApplication.currentApplication.finishedLaunching&&NSApp.activationPolicy==NSApplicationActivationPolicyProhibited){[NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];[c->window orderFront:nil];[NSApp updateWindows];}out[0]=(uint32_t)c->window.windowNumber;out[1]=c->window.isVisible;out[2]=c->window.isKeyWindow;out[3]=NSApp.isActive;out[4]=out[5]=0;CFArrayRef list=CGWindowListCopyWindowInfo(kCGWindowListOptionIncludingWindow,out[0]);for(NSDictionary*d in (NSArray*)list)if([d[(id)kCGWindowNumber]unsignedIntValue]==out[0]){out[4]=[d[(id)kCGWindowIsOnscreen]boolValue];out[5]=[d[(id)kCGWindowOwnerPID]unsignedIntValue];}if(list)CFRelease(list);return 1;}\n'
src=R/'samples/stage44_window.m';src.write_text(body)
loader=R/'samples/stage44_window.py';loader.write_text((R/'samples/stage31_window.py').read_text().replace('stage31-native/libstage31_window','stage44-native/libstage44_window').replace('stage31-window-build','stage44-window-build'))
lib=R/'runs/stage44-native/libstage44_window.dylib';lib.parent.mkdir(exist_ok=True)
cmd=['<LOCAL_MACHINE_PATH>','-arch','arm64','-isysroot','<LOCAL_MACHINE_PATH>','-O2','-Wall','-Wextra','-dynamiclib',str(src),'-framework','AppKit','-framework','CoreGraphics','-o',str(lib)]
subprocess.run(cmd,check=True,timeout=20)
out=dict(library=str(lib),sha256=hashlib.sha256(lib.read_bytes()).hexdigest(),source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),loader_sha256=hashlib.sha256(loader.read_bytes()).hexdigest(),command=cmd,scope='own background nonactivating NSPanel; initial Prohibited policy, never activate')
(R/'evidence/stage44-window-build.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
