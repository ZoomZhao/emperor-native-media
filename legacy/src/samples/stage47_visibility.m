#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#include <stdint.h>
#include <string.h>
static __weak NSWindow *watched;
static id closeObserver;
static uint32_t closeCount;
int s47_watch(void *pointer) {
 if (!pointer || !NSThread.isMainThread || closeObserver) return 0;
 watched=(__bridge NSWindow*)pointer;
 closeObserver=[NSNotificationCenter.defaultCenter addObserverForName:NSWindowWillCloseNotification object:watched queue:nil usingBlock:^(NSNotification *note){(void)note;closeCount++;}];
 return closeObserver!=nil;
}
int s47_lifecycle(uint32_t *out) { if(!out)return 0;out[0]=closeCount;out[1]=watched!=nil;return 1; }
/* Only the existing task-owned nonactivating panel may be reordered. */
int s47_recover(void *pointer, uint32_t guest_visible, uint32_t *out) {
 if (!pointer || !out || !NSThread.isMainThread || guest_visible > 1) return 0;
 NSWindow *win=(__bridge NSWindow*)pointer;
 if (![win isKindOfClass:NSPanel.class] || !(win.styleMask&NSWindowStyleMaskNonactivatingPanel) || win.canBecomeKeyWindow || win.canBecomeMainWindow || win.isMiniaturized || NSApp.isActive || win.isKeyWindow) return 0;
 BOOL beforeVisible=win.isVisible, beforeActive=NSApp.isActive, beforeKey=win.isKeyWindow;
 uint32_t policy=(uint32_t)NSApp.activationPolicy, launched=NSRunningApplication.currentApplication.finishedLaunching, action=0;
 if (guest_visible && launched && policy==NSApplicationActivationPolicyAccessory && !beforeVisible) {
  [win orderFront:nil]; [NSApp updateWindows]; [win displayIfNeeded]; action=1;
  [[NSRunLoop currentRunLoop]runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.002]];
 }
 uint32_t screen=0,owner=0,found=0;
 CFArrayRef list=CGWindowListCopyWindowInfo(kCGWindowListOptionIncludingWindow,(CGWindowID)win.windowNumber);
 for (NSDictionary *d in (__bridge NSArray*)list) if ([d[(id)kCGWindowNumber]unsignedIntValue]==win.windowNumber) {screen=[d[(id)kCGWindowIsOnscreen]boolValue];owner=[d[(id)kCGWindowOwnerPID]unsignedIntValue];found=1;}
 if (list)CFRelease(list);
 uint32_t values[12]={beforeVisible,policy,launched,action,(uint32_t)win.isVisible,(uint32_t)win.isKeyWindow,(uint32_t)NSApp.isActive,(uint32_t)win.windowNumber,screen,owner,found,(uint32_t)NSWorkspace.sharedWorkspace.frontmostApplication.processIdentifier};memcpy(out,values,sizeof(values));
 return beforeActive==NSApp.isActive && beforeKey==win.isKeyWindow;
}
