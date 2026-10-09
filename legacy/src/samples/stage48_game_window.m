#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#include <stdint.h>
#include <string.h>
static BOOL owned(NSWindow *win){return win&&[win isKindOfClass:NSPanel.class]&&(win.styleMask&NSWindowStyleMaskNonactivatingPanel)&&!win.canBecomeKeyWindow&&!win.canBecomeMainWindow&&!win.isMiniaturized&&!NSApp.isActive&&!win.isKeyWindow;}
/* Pump only reviewed lifecycle notifications for this actual game panel. */
int s48_launch(void *pointer,uint32_t *out){
 if(!pointer||!out||!NSThread.isMainThread)return 0;NSWindow *win=(__bridge NSWindow*)pointer;if(!owned(win))return 0;NSRect frame=win.frame;uint32_t count=0,type=0,subtype=0;
 NSDate *deadline=[NSDate dateWithTimeIntervalSinceNow:4];
 while(!NSRunningApplication.currentApplication.finishedLaunching&&deadline.timeIntervalSinceNow>0){
  NSEvent *event=[NSApp nextEventMatchingMask:NSEventMaskAppKitDefined untilDate:[NSDate dateWithTimeIntervalSinceNow:0.02] inMode:NSDefaultRunLoopMode dequeue:YES];
  if(event){type=(uint32_t)event.type;subtype=(uint32_t)event.subtype;
   if(event.window!=win||event.type!=NSEventTypeAppKitDefined||event.subtype!=NSEventSubtypeWindowMoved||!NSEqualRects(frame,win.frame)){[NSApp postEvent:event atStart:YES];out[0]=count;out[1]=type;out[2]=subtype;out[3]=0;return -1;}
   [NSApp sendEvent:event];count++;
  }
  [NSApp updateWindows];if(!owned(win))return -2;
 }
 uint32_t v[8]={count,type,subtype,(uint32_t)NSRunningApplication.currentApplication.finishedLaunching,(uint32_t)NSApp.activationPolicy,(uint32_t)win.isVisible,(uint32_t)NSApp.isActive,(uint32_t)win.isKeyWindow};memcpy(out,v,sizeof(v));return 1;
}
/* Diagnostic fault acts only on host visibility; guest memory is untouched. */
int s48_hide(void *pointer,uint32_t *out){
 if(!pointer||!out||!NSThread.isMainThread)return 0;NSWindow *win=(__bridge NSWindow*)pointer;if(!owned(win)||!NSRunningApplication.currentApplication.finishedLaunching||NSApp.activationPolicy!=NSApplicationActivationPolicyAccessory||!win.isVisible)return 0;
 uint32_t before=(uint32_t)NSWorkspace.sharedWorkspace.frontmostApplication.processIdentifier;[win orderOut:nil];[NSApp updateWindows];
 uint32_t v[8]={(uint32_t)win.isVisible,(uint32_t)win.isKeyWindow,(uint32_t)NSApp.isActive,before,(uint32_t)NSWorkspace.sharedWorkspace.frontmostApplication.processIdentifier,(uint32_t)win.windowNumber,(uint32_t)NSApp.activationPolicy,(uint32_t)NSRunningApplication.currentApplication.finishedLaunching};memcpy(out,v,sizeof(v));return !win.isVisible&&!win.isKeyWindow&&!NSApp.isActive&&v[3]==v[4];
}
/* Read-only screenshot content check, independent of WindowServer metadata. */
int s48_png(const char *path,uint32_t *out){@autoreleasepool{
 if(!path||!out)return 0;NSData *data=[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:path]];NSBitmapImageRep *image=[[NSBitmapImageRep alloc]initWithData:data];if(!image||image.pixelsWide<64||image.pixelsHigh<64)return 0;
 uint32_t samples=0,rgb=0,alpha=0,opaque=0;
 for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){
  NSInteger px=image.pixelsWide*(x+1)/17,py=image.pixelsHigh*(y+1)/17;NSColor *c=[[image colorAtX:px y:py]colorUsingColorSpace:NSColorSpace.deviceRGBColorSpace];if(!c)return 0;CGFloat r,g,b,a;[c getRed:&r green:&g blue:&b alpha:&a];samples++;rgb+=(r+g+b)>0.01;alpha+=a>0.01;opaque+=a>0.99;
 }
 uint32_t v[8]={(uint32_t)image.pixelsWide,(uint32_t)image.pixelsHigh,(uint32_t)image.bitsPerPixel,(uint32_t)image.samplesPerPixel,samples,rgb,alpha,opaque};memcpy(out,v,sizeof(v));return 1;
}}
