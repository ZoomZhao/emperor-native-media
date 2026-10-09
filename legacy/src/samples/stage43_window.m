/* Real AppKit objects plus a task-local Win32 class/handle namespace. */
#import <AppKit/AppKit.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
@interface S43Window:NSWindow
@end
@implementation S43Window
/* Guest window coordinates belong to this isolated compatibility domain. */
- (NSRect)constrainFrameRect:(NSRect)frameRect toScreen:(NSScreen*)screen{(void)screen;return frameRect;}
- (BOOL)canBecomeKeyWindow{return YES;}
- (BOOL)canBecomeMainWindow{return YES;}
@end
enum{CAP=8,DATA=8192};
typedef struct{uint32_t id,kind,x,y,n,pad;unsigned char data[DATA];}Resource;
typedef struct{uint32_t atom,words[9];char name[128];}GuestClass;
typedef struct{uint32_t handle,tid,creating,style,exstyle;int32_t x,y,w,h;char title[256],class_name[128];}Window;
typedef struct{uint32_t magic,version,next,pad;Resource resources[CAP];GuestClass cls;Window window;}Image;
typedef struct{uint32_t magic,version,created,visible,active,focus,dc_live,dc_id,w,h,pitch,owner,reserved[4];}LifeImage;
typedef struct{Image im;id objects[CAP];NSWindow*window;LifeImage life;CGContextRef dc;unsigned char*pixels;uint32_t requested,policy_ok,event_count,event_types[32],event_subtypes[32],background_test;}Core;
static id make(Resource*r){
 if(r->kind==1){NSData*d=[NSData dataWithBytes:r->data length:r->n];NSImage*i=[[NSImage alloc]initWithData:d];if(i.size.width<=0||i.size.height<=0){[i release];return nil;}return i;}
 if(r->kind==2)return[NSColor.blackColor retain];
 if(r->kind==3){
  if(r->n!=256||r->x>=32||r->y>=32)return nil;
  NSBitmapImageRep*b=[[NSBitmapImageRep alloc]initWithBitmapDataPlanes:NULL pixelsWide:32 pixelsHigh:32 bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:128 bitsPerPixel:32];if(!b)return nil;
  for(unsigned i=0;i<1024;i++){unsigned a=(r->data[i/8]>>(7-i%8))&1,x=(r->data[128+i/8]>>(7-i%8))&1;uint8_t*p=b.bitmapData+4*i;if(a&&x){[b release];return nil;}p[0]=p[1]=p[2]=x?255:0;p[3]=a?0:255;}
  NSImage*im=[[NSImage alloc]initWithSize:NSMakeSize(32,32)];[im addRepresentation:b];NSCursor*c=[[NSCursor alloc]initWithImage:im hotSpot:NSMakePoint(r->x,r->y)];[im release];[b release];return c;
 }
 return nil;
}
static int style_mask(uint32_t style,NSWindowStyleMask*out){if(style==0x90080000u){*out=NSWindowStyleMaskBorderless;return 1;}if(style==0x10ca0000u){*out=NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable;return 1;}return 0;}
static NSWindow*make_window(Window*w){
 if(!NSThread.isMainThread||w->w<=0||w->h<=0||w->exstyle!=0x40000u)return nil;
 NSScreen*s=NSScreen.mainScreen;if(!s)return nil;NSRect frame=NSMakeRect(w->x,s.frame.size.height-w->y-w->h,w->w,w->h);
 NSWindowStyleMask mask;if(!style_mask(w->style,&mask))return nil;NSRect content=[NSWindow contentRectForFrameRect:frame styleMask:mask];NSWindow*win=[[S43Window alloc]initWithContentRect:content styleMask:mask backing:NSBackingStoreBuffered defer:NO];[win setFrame:frame display:NO];win.releasedWhenClosed=NO;win.title=[NSString stringWithUTF8String:w->title];win.backgroundColor=NSColor.blackColor;return win;
}
void*s26_window_new(void){if(!NSThread.isMainThread)return NULL;[NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];Core*c=calloc(1,sizeof(*c));if(!c)return NULL;c->im.magic=0x57494e32;c->im.version=1;c->im.next=1;c->life.magic=0x4c493237;c->life.version=1;return c;}
void s26_window_delete(Core*c){if(c){for(unsigned i=0;i<CAP;i++)[c->objects[i]release];[c->window close];[c->window release];if(c->dc)CGContextRelease(c->dc);free(c->pixels);free(c);}}
uint32_t s26_window_resource(Core*c,uint32_t kind,uint32_t x,uint32_t y,const void*p,size_t n){
 if(!c||n>DATA||(n&&!p))return 0;for(unsigned i=0;i<CAP;i++)if(c->im.resources[i].id){Resource*r=c->im.resources+i;if(r->kind==kind&&r->x==x&&r->y==y&&r->n==n&&!memcmp(r->data,p,n))return r->id;}
 for(unsigned i=0;i<CAP;i++)if(!c->im.resources[i].id){Resource r={0};r.id=0x57000000u|c->im.next;r.kind=kind;r.x=x;r.y=y;r.n=(uint32_t)n;if(n)memcpy(r.data,p,n);id object=make(&r);if(!object)return 0;c->im.next++;c->im.resources[i]=r;c->objects[i]=object;return r.id;}return 0;
}
uint32_t s26_window_kind(Core*c,uint32_t h){for(unsigned i=0;i<CAP;i++)if(c->im.resources[i].id==h&&c->objects[i])return c->im.resources[i].kind;return 0;}
uint32_t s26_window_register(Core*c,const uint32_t*words,const char*name,uint32_t*error){
 if(!c||!words||!name||strlen(name)>=128)return 0;if(c->im.cls.atom){*error=1410;return 0;}
 if(words[0]||!words[1]||words[2]||words[3]||words[4]!=0x400000||words[6]||s26_window_kind(c,words[5])!=1||s26_window_kind(c,words[7])!=2||words[8])return 0;
 c->im.cls.atom=0xc001;memcpy(c->im.cls.words,words,9*4);strcpy(c->im.cls.name,name);return c->im.cls.atom;
}
uint32_t s26_window_find(Core*c,const char*cls,const char*title){if(!c->window)return 0;Window*w=&c->im.window;if((cls&&strcasecmp(cls,w->class_name))||(title&&strcasecmp(title,w->title)))return 0;return w->handle;}
uint32_t s26_window_begin(Core*c,uint32_t tid,const char*cls,const char*title,const uint32_t*a){
 if(!c||!a||!tid||!cls||!title||strlen(title)>=256||c->window||!c->im.cls.atom||strcmp(cls,c->im.cls.name)||a[8]||a[9]||a[10]!=c->im.cls.words[4])return 0;
 Window w={0};w.handle=0x58000001;w.tid=tid;w.creating=1;w.exstyle=a[0];w.style=a[3];w.x=(int32_t)a[4];w.y=(int32_t)a[5];w.w=(int32_t)a[6];w.h=(int32_t)a[7];strcpy(w.title,title);strcpy(w.class_name,cls);NSWindow*win=make_window(&w);if(!win)return 0;c->im.window=w;c->window=win;return w.handle;
}
void*s26_window_pointer(Core*c,uint32_t handle){return c&&c->im.window.handle==handle?c->window:NULL;}
size_t s26_window_size(void){return sizeof(Image);}
int s26_window_export(Core*c,void*p,size_t n){if(!c||n!=sizeof(Image))return 0;memcpy(p,&c->im,n);return 1;}
int s26_window_import(Core*c,const void*p,size_t n){
 if(!c||n!=sizeof(Image))return 0;Image*im=malloc(n);if(!im)return 0;memcpy(im,p,n);id objects[CAP]={0};NSWindow*win=nil;int okay=im->magic==0x57494e32&&im->version==1&&im->next>=1&&im->next<=CAP+1&&!im->pad;
 for(unsigned i=0;i<CAP&&okay;i++){Resource*r=im->resources+i;if(r->id){if((r->id&0xff000000u)!=0x57000000u||(r->id&0xffffffu)>=im->next||r->pad||r->n>DATA)okay=0;else{objects[i]=make(r);if(!objects[i])okay=0;}for(unsigned j=0;j<i;j++)if(r->id==im->resources[j].id)okay=0;}}
 if(okay&&im->window.handle){if(im->window.handle!=0x58000001||im->window.creating<1||im->window.creating>3||!memchr(im->window.title,0,256)||!memchr(im->window.class_name,0,128))okay=0;else{win=make_window(&im->window);if(!win)okay=0;}}
 if(!okay){for(unsigned i=0;i<CAP;i++)[objects[i]release];[win close];[win release];free(im);return 0;}
 for(unsigned i=0;i<CAP;i++)[c->objects[i]release];[c->window close];[c->window release];memcpy(c->objects,objects,sizeof(objects));c->window=win;c->im=*im;free(im);return 1;
}
/* The only keyboard projection implemented here is the reviewed VK_MENU high
 * SHORT bit. The original caller clears its low byte at 0x43cfad. */
int s27_alt_state(uint32_t*out,uint64_t*flags){
 if(!out||!flags||!NSThread.isMainThread)return 0;
 for(unsigned i=0;i<3;i++){NSEventModifierFlags a=NSEvent.modifierFlags;CGEventFlags g=CGEventSourceFlagsState(kCGEventSourceStateCombinedSessionState);bool left=CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState,58),right=CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState,61);flags[0]=a;flags[1]=g;flags[2]=left;flags[3]=right;unsigned down=(a&NSEventModifierFlagOption)!=0;if(down==((g&kCGEventFlagMaskAlternate)!=0)&&down==(unsigned)(left||right)){*out=down?0xffff8000u:0;return 1;}}
 return 0;
}
static int live(Core*c,uint32_t h){return c&&c->window&&c->im.window.handle==h;}
static int ensure_dc(Core*c){
 if(!c->window||c->life.reserved[1])return 0;NSRect content=[c->window contentRectForFrameRect:c->window.frame];unsigned w=(unsigned)content.size.width,h=(unsigned)content.size.height;if(c->dc&&c->life.w==w&&c->life.h==h)return 1;if(!w||!h||w>4096||h>4096)return 0;
 unsigned char*newpixels=calloc((size_t)w*h,4);if(!newpixels)return 0;CGColorSpaceRef color=CGColorSpaceCreateDeviceRGB();CGContextRef dc=CGBitmapContextCreate(newpixels,w,h,8,w*4,color,kCGImageAlphaPremultipliedFirst|kCGBitmapByteOrder32Little);CGColorSpaceRelease(color);if(!dc){free(newpixels);return 0;}
 if(c->dc){unsigned cw=MIN(w,c->life.w),ch=MIN(h,c->life.h);for(unsigned y=0;y<ch;y++)memcpy(newpixels+y*w*4,c->pixels+y*c->life.pitch,cw*4);CGContextRelease(c->dc);free(c->pixels);}c->dc=dc;c->pixels=newpixels;c->life.dc_live=1;c->life.dc_id=0x59000001;c->life.w=w;c->life.h=h;c->life.pitch=w*4;return 1;
}
int s27_window_created(Core*c,uint32_t h){if(!live(c,h))return 0;c->life.created=1;c->life.owner=c->im.window.tid;c->im.window.creating=2;return 1;}
int s27_window_show(Core*c,uint32_t h){if(!live(c,h)||!c->life.created)return 0;c->policy_ok=[NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];[NSApp finishLaunching];[c->window orderFront:nil];[c->window displayIfNeeded];if(!c->window.isVisible)return 0;c->life.visible=1;c->life.reserved[0]=1;c->life.reserved[2]=1;c->im.window.creating=3;return 1;}
/* One activation request per restored native process. Polling consumes only
 * AppKit lifecycle events; user input is left queued for the guest adapter. */
int s27_window_activate(Core*c,uint32_t h){if(!live(c,h)||!c->life.visible)return 0;if(c->background_test)return NSApp.isActive&&c->window.isKeyWindow;if(!c->requested){c->requested=1;[NSApp activateIgnoringOtherApps:YES];[c->window makeKeyWindow];}NSDate*limit=[NSDate dateWithTimeIntervalSinceNow:0.5];while((!NSApp.isActive||!c->window.isKeyWindow)&&limit.timeIntervalSinceNow>0){NSEvent*event=[NSApp nextEventMatchingMask:NSEventMaskAppKitDefined untilDate:[NSDate dateWithTimeIntervalSinceNow:0.01] inMode:NSDefaultRunLoopMode dequeue:YES];if(event){if(c->event_count<32){c->event_types[c->event_count]=(uint32_t)event.type;c->event_subtypes[c->event_count]=(uint32_t)event.subtype;c->event_count++;}[NSApp sendEvent:event];}[NSApp updateWindows];}if(!NSApp.isActive||!c->window.isKeyWindow)return 0;c->life.active=h;return 1;}
int s28_window_background_test(Core*c,uint32_t value){if(!c||value>1||c->requested)return 0;c->background_test=value;return 1;}
int s28_window_diagnostic(Core*c,uint32_t*out,size_t n){if(!c||!c->window||!out||n!=80)return 0;memset(out,0,n*4);NSRunningApplication*a=NSRunningApplication.currentApplication;out[0]=NSThread.isMainThread;out[1]=(uint32_t)NSApp.activationPolicy;out[2]=c->policy_ok;out[3]=NSApp.isActive;out[4]=NSApp.isRunning;out[5]=a.isFinishedLaunching;out[6]=a.isActive;out[7]=(uint32_t)a.activationPolicy;out[8]=c->window.isVisible;out[9]=c->window.isKeyWindow;out[10]=c->window.canBecomeKeyWindow;out[11]=c->window.canBecomeMainWindow;out[12]=c->requested;out[13]=c->event_count;out[14]=(uint32_t)a.processIdentifier;out[15]=(uint32_t)NSWorkspace.sharedWorkspace.frontmostApplication.processIdentifier;memcpy(out+16,c->event_types,32*4);memcpy(out+48,c->event_subtypes,32*4);return 1;}
int s27_window_focus(Core*c,uint32_t tid,uint32_t h,uint32_t*old){if(!live(c,h)||tid!=c->im.window.tid||!c->life.active||!old)return 0;*old=c->life.focus;[c->window makeFirstResponder:c->window.contentView];if(!c->window.firstResponder)return 0;c->life.focus=h;return 1;}
int s27_window_query(Core*c,uint32_t h,uint32_t*out,size_t n){if(!live(c,h)||n!=12)return 0;Window*w=&c->im.window;uint32_t a[12]={w->tid,(uint32_t)w->x,(uint32_t)w->y,(uint32_t)w->w,(uint32_t)w->h,w->style,w->exstyle,c->life.active,c->life.focus,c->window.isVisible,c->window.isKeyWindow,c->window.isMiniaturized};memcpy(out,a,sizeof(a));return 1;}
uint32_t s27_window_foreground(Core*c){return c&&c->window&&NSApp.isActive&&c->window.isVisible&&c->window.isKeyWindow?c->im.window.handle:0;}
int s27_window_rect(Core*c,uint32_t h,uint32_t client,int32_t*out){if(!live(c,h)||!out)return 0;Window*w=&c->im.window;NSRect rect=client?[c->window contentRectForFrameRect:c->window.frame]:c->window.frame;out[0]=client?0:w->x;out[1]=client?0:w->y;out[2]=out[0]+(int32_t)rect.size.width;out[3]=out[1]+(int32_t)rect.size.height;return 1;}
uint32_t s27_window_dc(Core*c,uint32_t h){return(h==0||live(c,h))&&ensure_dc(c)?c->life.dc_id:0;}
int s27_window_default(Core*c,uint32_t h,uint32_t msg,uint32_t wp,void*data,size_t n,uint32_t*out){
 if(!live(c,h)||!out)return 0;Window*w=&c->im.window;*out=0;
 if(msg==0x81||msg==1){if(wp||n!=48||!data)return 0;uint32_t*a=data;if(a[1]!=c->im.cls.words[4]||a[2]||a[3]||a[4]!=(uint32_t)w->h||a[5]!=(uint32_t)w->w||a[6]!=(uint32_t)w->y||a[7]!=(uint32_t)w->x||a[8]!=w->style||a[11]!=w->exstyle)return 0;*out=msg==0x81;return 1;}
 if(msg==0x7c||msg==0x7d){if(wp!=0xfffffff0u||n!=8||!data)return 0;return 1;}
 if(msg==0x83){if(wp||n!=16||!data)return 0;int32_t*r=data;r[0]=w->x;r[1]=w->y;r[2]=w->x+w->w;r[3]=w->y+w->h;return 1;}
 if(msg==0x14){if(n||wp!=c->life.dc_id||!ensure_dc(c))return 0;CGContextSetRGBFillColor(c->dc,0,0,0,1);CGContextFillRect(c->dc,CGRectMake(0,0,w->w,w->h));c->window.backgroundColor=NSColor.blackColor;[c->window displayIfNeeded];c->life.reserved[2]=0;*out=1;return 1;}
 if(msg==0x86){if(wp>1||n)return 0;*out=1;return 1;}
 /* Review-bounded notification defaults. Their effects are performed by the
  * owner lifecycle before dispatch, not fabricated by these return values. */
 if(msg==3||msg==5||msg==7||msg==8||msg==0x18||msg==0x1c||msg==6||msg==0x30f||msg==0x85){if(n)return 0;return 1;}
 if(msg==0xf){if(n||wp||c->life.reserved[0]||c->life.reserved[1])return 0;return 1;}
 if(msg==0x46||msg==0x47){if(wp||n!=28||!data||((uint32_t*)data)[0]!=h)return 0;uint32_t flags=((uint32_t*)data)[6];if(msg==0x46&&(flags&3)!=3)return 0;if(msg==0x47&&(flags&0x1800)!=0x1800)return 0;return 1;}
 return 0;
}
size_t s27_life_size(Core*c){return sizeof(LifeImage)+(c->life.dc_live?(size_t)c->life.pitch*c->life.h:0);}
int s27_life_export(Core*c,void*p,size_t n){if(!c||n!=s27_life_size(c))return 0;memcpy(p,&c->life,sizeof(LifeImage));if(c->life.dc_live)memcpy((unsigned char*)p+sizeof(LifeImage),c->pixels,n-sizeof(LifeImage));return 1;}
int s27_life_import(Core*c,const void*p,size_t n){
 if(!c||n<sizeof(LifeImage))return 0;LifeImage im;memcpy(&im,p,sizeof(im));if(im.magic!=0x4c493237||im.version!=1||im.created>1||im.visible>1||im.dc_live>1||im.reserved[0]>1||im.reserved[1]>1||im.reserved[2]>1||im.reserved[3])return 0;
 if(im.dc_live){if(!ensure_dc(c)||im.dc_id!=c->life.dc_id||im.w!=c->life.w||im.h!=c->life.h||im.pitch!=c->life.pitch||n!=sizeof(im)+(size_t)im.pitch*im.h)return 0;memcpy(c->pixels,(const unsigned char*)p+sizeof(im),n-sizeof(im));}else if(n!=sizeof(im))return 0;
 c->life=im;if(im.visible&&!s27_window_show(c,c->im.window.handle))return 0;if(im.active&&!s27_window_activate(c,im.active))return 0;if(im.focus){uint32_t old;if(!s27_window_focus(c,c->im.window.tid,im.focus,&old))return 0;}c->life=im;if(im.reserved[1]){CGContextSaveGState(c->dc);CGContextClipToRect(c->dc,CGRectMake(0,0,im.w,im.h));}return 1;
}
uint32_t s27_paint_dirty(Core*c){return c?c->life.reserved[0]:0;}
uint32_t s27_paint_begin(Core*c,uint32_t h,uint32_t*out,size_t n){if(!live(c,h)||n!=16||!out||!ensure_dc(c)||c->life.reserved[1]||c->life.reserved[2])return 0;memset(out,0,64);out[0]=c->life.dc_id;if(c->life.reserved[0]){out[4]=c->life.w;out[5]=c->life.h;}CGContextSaveGState(c->dc);CGContextClipToRect(c->dc,CGRectMake(0,0,out[4],out[5]));c->life.reserved[0]=0;c->life.reserved[1]=1;return c->life.dc_id;}
int s27_paint_end(Core*c,uint32_t h,uint32_t dc){if(!live(c,h)||!c->life.reserved[1]||dc!=c->life.dc_id)return 0;CGContextRestoreGState(c->dc);c->life.reserved[1]=0;return 1;}
int s28_paint_invalidate(Core*c,uint32_t h,uint32_t erase){if(!live(c,h)||erase>1||c->life.reserved[1])return 0;c->life.reserved[0]=1;if(erase)c->life.reserved[2]=1;[c->window.contentView setNeedsDisplay:YES];return 1;}
int s28_window_close(Core*c){if(!c)return 0;if(!c->window)return 1;[c->window orderOut:nil];[c->window close];return !c->window.isVisible;}

/* Only the reviewed single-window style transition, always preserving its
 * actual outer frame and key state. No activation or physical display call. */
int s30_window_style(Core*c,uint32_t h,uint32_t style){
 if(!live(c,h)||c->life.reserved[1])return 0;NSWindowStyleMask mask;if(!style_mask(style,&mask))return 0;NSRect frame=c->window.frame;BOOL key=c->window.isKeyWindow;[c->window setStyleMask:mask];[c->window setFrame:frame display:NO];if(c->window.isKeyWindow!=key)return 0;c->im.window.style=style;return !c->life.dc_live||ensure_dc(c);
}
int s30_adjust_rect(Core*c,uint32_t style,uint32_t menu,uint32_t exstyle,int32_t*r){
 NSWindowStyleMask mask;if(!c||!r||menu||exstyle!=0x40000u||!style_mask(style,&mask)||r[2]<r[0]||r[3]<r[1])return 0;NSRect box=NSMakeRect(r[0],-r[3],r[2]-r[0],r[3]-r[1]);NSRect frame=[NSWindow frameRectForContentRect:box styleMask:mask];r[0]=(int32_t)frame.origin.x;r[1]=-(int32_t)NSMaxY(frame);r[2]=(int32_t)NSMaxX(frame);r[3]=-(int32_t)frame.origin.y;return 1;
}
int s30_window_menu(Core*c,uint32_t h,uint32_t*out){if(!live(c,h)||!out)return 0;/* this native window was created with no guest menu */*out=0;return 1;}

int s30_geometry_report(Core*c,uint32_t h,int32_t*out){
 if(!live(c,h)||!out||!c->window.screen||!c->dc)return 0;NSRect frame=c->window.frame,content=[c->window contentRectForFrameRect:frame];CGFloat top=NSMaxY(c->window.screen.frame);int32_t a[13]={(int32_t)frame.origin.x,(int32_t)(top-NSMaxY(frame)),(int32_t)frame.size.width,(int32_t)frame.size.height,(int32_t)content.origin.x,(int32_t)(top-NSMaxY(content)),(int32_t)content.size.width,(int32_t)content.size.height,(int32_t)c->window.styleMask,(int32_t)CGBitmapContextGetBitsPerPixel(c->dc),(int32_t)CGBitmapContextGetBytesPerRow(c->dc),(int32_t)CGBitmapContextGetWidth(c->dc),(int32_t)CGBitmapContextGetHeight(c->dc)};memcpy(out,a,sizeof(a));return 1;
}

int s31_work_area(Core*c,int32_t*out){if(!c||!out||!c->window.screen)return 0;NSScreen*s=c->window.screen;NSRect r=s.visibleFrame;out[0]=(int32_t)r.origin.x;out[1]=(int32_t)(NSMaxY(s.frame)-NSMaxY(r));out[2]=(int32_t)NSMaxX(r);out[3]=(int32_t)(NSMaxY(s.frame)-r.origin.y);return 1;}
int s31_position(Core*c,uint32_t h,const uint32_t*a){
 if(!live(c,h)||!a||a[0]!=h||c->life.reserved[1]||!(a[6]&0x10)||(a[6]&~0x17u))return 0;if(!(a[6]&4)&&a[1]!=0xfffffffeu)return 0;
 Window*w=&c->im.window;int32_t x=a[6]&2?w->x:(int32_t)a[2],y=a[6]&2?w->y:(int32_t)a[3],width=a[6]&1?w->w:(int32_t)a[4],height=a[6]&1?w->h:(int32_t)a[5];if(width<=0||height<=28||width>4096||height>4096)return 0;BOOL key=c->window.isKeyWindow,active=NSApp.isActive;NSRect frame=NSMakeRect(x,NSMaxY(c->window.screen.frame)-y-height,width,height);BOOL resized=w->w!=width||w->h!=height;[c->window setFrame:frame display:NO];if(!(a[6]&4))c->window.level=NSNormalWindowLevel;if(key!=c->window.isKeyWindow||active!=NSApp.isActive)return 0;w->x=x;w->y=y;w->w=width;w->h=height;if(!ensure_dc(c))return 0;if(resized){c->life.reserved[0]=1;c->life.reserved[2]=1;}[c->window displayIfNeeded];return 1;
}
int s31_finite_default(Core*c,uint32_t h,uint32_t msg,uint32_t wp,void*data,size_t n,uint32_t*out){
 if(!live(c,h)||!out)return 0;*out=0;
 if(msg==0x24){if(wp||n!=40||!data)return 0;return 1;}
 if(msg==0x83&&wp==1){if(n!=52||!data)return 0;int32_t*r=data;NSScreen*s=c->window.screen;NSRect box=NSMakeRect(r[0],NSMaxY(s.frame)-r[3],r[2]-r[0],r[3]-r[1]);NSRect client=[NSWindow contentRectForFrameRect:box styleMask:c->window.styleMask];r[0]=(int32_t)client.origin.x;r[1]=(int32_t)(NSMaxY(s.frame)-NSMaxY(client));r[2]=(int32_t)NSMaxX(client);r[3]=(int32_t)(NSMaxY(s.frame)-client.origin.y);return 1;}
 if(msg==0x46||msg==0x47){if(wp||n!=28||!data||((uint32_t*)data)[0]!=h)return 0;return 1;}
 return s27_window_default(c,h,msg,wp,data,n,out);
}

int s31_minmax(Core*c,uint32_t h,int32_t*a){if(!live(c,h)||!a)return 0;int32_t r[4];if(!s31_work_area(c,r))return 0;NSSize min=c->window.minSize,max=c->window.maxSize;a[0]=a[1]=0;a[2]=r[2]-r[0];a[3]=r[3]-r[1];a[4]=r[0];a[5]=r[1];a[6]=(int32_t)MAX(min.width,[NSWindow minFrameWidthWithTitle:c->window.title styleMask:c->window.styleMask]);a[7]=(int32_t)MAX(min.height,28);a[8]=(int32_t)MIN(max.width,4096);a[9]=(int32_t)MIN(max.height,4096);return 1;}

int s31_existing_directory(const char*root,const char*child,uint32_t*out){
 if(!root||!child||!out||(strcmp(child,"Save")&&strcmp(child,"Campaigns")))return 0;int fd=open(root,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(fd<0)return 0;int item=openat(fd,child,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);close(fd);if(item<0)return 0;struct stat st;int okay=!fstat(item,&st)&&S_ISDIR(st.st_mode);close(item);if(!okay)return 0;*out=183;return 1;
}
