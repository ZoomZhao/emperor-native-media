/* Authored readonly AppKit coordinate adapter. No posting, focus or warp APIs. */
#import <AppKit/AppKit.h>
#include <stdint.h>
#include <math.h>
static int scope(NSWindow*w){return w&&NSThread.isMainThread&&NSScreen.screens.count==1&&w.screen&&w.contentView;}
static void diagnostic(NSWindow*w,double*out){
 NSRect c=[w contentRectForFrameRect:w.frame],b=[w.contentView convertRectToBacking:w.contentView.bounds];
 double a[12]={NSMaxY(NSScreen.screens[0].frame),c.origin.x,c.origin.y,c.size.width,c.size.height,w.backingScaleFactor,b.size.width,b.size.height,NSApp.isActive,w.isKeyWindow,NSScreen.screens.count,w.contentView.isFlipped};
 memcpy(out,a,sizeof(a));
}
int s43_cursor(void*p,int32_t*out,double*diag){NSWindow*w=(__bridge NSWindow*)p;if(!scope(w)||!out||!diag)return 0;BOOL active=NSApp.isActive,key=w.isKeyWindow;NSPoint q=NSEvent.mouseLocation;out[0]=(int32_t)floor(q.x);out[1]=(int32_t)floor(NSMaxY(NSScreen.screens[0].frame)-q.y);diagnostic(w,diag);diag[12]=q.x;diag[13]=q.y;return active==NSApp.isActive&&key==w.isKeyWindow;}
int s43_convert(void*p,uint32_t toScreen,const int32_t*in,int32_t*out,double*diag){NSWindow*w=(__bridge NSWindow*)p;if(!scope(w)||!in||!out||!diag||toScreen>1)return 0;BOOL active=NSApp.isActive,key=w.isKeyWindow;NSView*v=w.contentView;CGFloat top=NSMaxY(NSScreen.screens[0].frame);NSPoint q;
 if(toScreen){q=NSMakePoint(in[0],v.isFlipped?in[1]:v.bounds.size.height-in[1]);q=[w convertPointToScreen:[v convertPoint:q toView:nil]];q.y=top-q.y;}
 else{q=NSMakePoint(in[0],top-in[1]);q=[v convertPoint:[w convertPointFromScreen:q] fromView:nil];if(!v.isFlipped)q.y=v.bounds.size.height-q.y;}
 out[0]=(int32_t)floor(q.x);out[1]=(int32_t)floor(q.y);diagnostic(w,diag);diag[12]=q.x;diag[13]=q.y;return active==NSApp.isActive&&key==w.isKeyWindow;
}
