/* Finite port of confirmed metadata/resource consumers; no window or pump.
 * Guest pixels use AppKit logical screen units; backing-pixel DPI is converted
 * by real backingScaleFactor. Test profile is explicit and never live fallback.
 * DC retains the actual display-mode object for information queries only.
 * Brushes own real CGColor objects. Cursors own real NSCursor objects; wait
 * image is an explicitly task-authored platform fallback, not Windows art.
 */
#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <CoreGraphics/CoreGraphics.h>
#include "stage15_gui.h"
#include <math.h>
#include <string.h>
enum {CAP=32,MAGIC=0x47553135,VERSION=1};
typedef struct {uint32_t id,type,role,live;} record;
typedef struct {uint32_t magic,version,fixture,next;uint32_t profile[S15_PROFILE_WORDS];record records[CAP];} image;
struct s15_gui {image data;CFTypeRef objects[CAP];};
static int color_index(uint32_t role) {switch(role) {case 15:return 0;case 16:return 1;case 20:return 2;case 18:return 3;case 6:return 4;default:return -1;}}
static uint32_t pack_color(NSColor *source) {
 NSColor *c=[source colorUsingColorSpace:NSColorSpace.deviceRGBColorSpace];if(!c)return UINT32_MAX;
 return (uint32_t)lround(c.redComponent*255)|((uint32_t)lround(c.greenComponent*255)<<8)|((uint32_t)lround(c.blueComponent*255)<<16);
}
static int live_profile(uint32_t p[S15_PROFILE_WORDS]) {@autoreleasepool {
 if(!NSThread.isMainThread)return 0;NSScreen *s=NSScreen.mainScreen;if(!s)return 0;
 CGDirectDisplayID display=[s.deviceDescription[@"NSScreenNumber"] unsignedIntValue];CGDisplayModeRef mode=CGDisplayCopyDisplayMode(display);if(!mode)return 0;
 NSSize icon=[NSWorkspace.sharedWorkspace iconForContentType:UTTypeFolder].size;
 NSSize dpi=[s.deviceDescription[NSDeviceResolution] sizeValue];CGFloat scale=s.backingScaleFactor;
 CGFloat scrollbar=[NSScroller scrollerWidthForControlSize:NSControlSizeRegular scrollerStyle:NSScrollerStyleLegacy];
 memset(p,0,S15_PROFILE_WORDS*4);p[0]=display;p[1]=(uint32_t)lround(s.frame.size.width);p[2]=(uint32_t)lround(s.frame.size.height);p[3]=(uint32_t)lround(icon.width);p[4]=(uint32_t)lround(icon.height);p[5]=(uint32_t)lround(scrollbar);
 p[6]=(uint32_t)lround(dpi.width/scale);p[7]=(uint32_t)lround(dpi.height/scale);p[8]=(uint32_t)lround(dpi.width);p[9]=(uint32_t)lround(dpi.height);p[10]=(uint32_t)lround(scale*1000);p[11]=(uint32_t)CGDisplayModeGetPixelWidth(mode);p[12]=(uint32_t)CGDisplayModeGetPixelHeight(mode);CGDisplayModeRelease(mode);
 // Finite native-theme role mapping; does not claim Windows pixel fidelity.
 p[13]=pack_color(NSColor.controlColor);p[14]=pack_color(NSColor.shadowColor);p[15]=pack_color(NSColor.separatorColor);p[16]=pack_color(NSColor.controlTextColor);p[17]=pack_color(NSColor.windowFrameTextColor);
 for(unsigned i=1;i<13;i++)if(!p[i])return 0;
 for(unsigned i=13;i<18;i++)if(p[i]>0xffffff)return 0;return 1;
}}
static int unchanged(s15_gui *s) {if(s->data.fixture)return 1;uint32_t p[S15_PROFILE_WORDS];return live_profile(p)&&!memcmp(p,s->data.profile,sizeof(p));}
static CFTypeRef object_for(s15_gui *s,uint32_t type,uint32_t role) {@autoreleasepool {
 if(type==1) {
  if(s->data.fixture)return CFNumberCreate(NULL,kCFNumberSInt32Type,&s->data.profile[0]);
  return CGDisplayCopyDisplayMode(s->data.profile[0]);
 }
 if(type==2) {
  int i=color_index(role);if(i<0)return NULL;uint32_t c=s->data.profile[13+i];CGFloat components[]={(c&255)/255.,((c>>8)&255)/255.,((c>>16)&255)/255.,1};CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();CGColorRef color=CGColorCreate(space,components);CGColorSpaceRelease(space);return color;
 }
 if(type==3) {
  if(role==32512)return CFBridgingRetain(NSCursor.arrowCursor);
  if(role!=32514)return NULL;
  // No public NSCursor wait factory in this installed SDK. Create a retained
  // real busy-role cursor from an own hourglass image. Never call -set here.
  // System arrow cursors may expose no NSImage; they still own a valid system
  // cursor. Our named fallback uses the actual system icon extent instead.
  NSSize size=NSMakeSize(s->data.profile[3],s->data.profile[4]);if(size.width<=0||size.height<=0)return NULL;
  NSImage *img=[[NSImage alloc] initWithSize:size];[img lockFocus];[NSColor.controlTextColor setFill];
  NSBezierPath *path=NSBezierPath.bezierPath;
  [path moveToPoint:NSMakePoint(size.width*.2,size.height*.85)];[path lineToPoint:NSMakePoint(size.width*.8,size.height*.85)];[path lineToPoint:NSMakePoint(size.width*.5,size.height*.5)];[path closePath];
  [path moveToPoint:NSMakePoint(size.width*.2,size.height*.15)];[path lineToPoint:NSMakePoint(size.width*.8,size.height*.15)];[path lineToPoint:NSMakePoint(size.width*.5,size.height*.5)];[path closePath];[path fill];[img unlockFocus];
  NSCursor *cursor=[[NSCursor alloc] initWithImage:img hotSpot:NSMakePoint(size.width*.5,size.height*.5)];return CFBridgingRetain(cursor);
 }
 return NULL;
}}
s15_gui *s15_gui_new(const uint32_t *fixture) {@autoreleasepool {
 if(!NSThread.isMainThread)return NULL;
 // A ctypes host has no application object; public system NSCursor factories
 // require AppKit application initialization. This process stays background.
 [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
 s15_gui *s=calloc(1,sizeof(*s));if(!s)return NULL;
 s->data.magic=MAGIC;s->data.version=VERSION;s->data.fixture=fixture!=NULL;s->data.next=1;
 if(fixture)memcpy(s->data.profile,fixture,S15_PROFILE_WORDS*4);
 else if(!live_profile(s->data.profile)) {free(s);return NULL;}
 return s;
}}
void s15_gui_delete(s15_gui *s) {for(unsigned i=0;i<CAP;i++)if(s->objects[i])CFRelease(s->objects[i]);free(s);}
static int find(s15_gui *s,uint32_t h) {for(unsigned i=0;i<CAP;i++)if(s->data.records[i].live&&s->data.records[i].id==h)return (int)i;return -1;}
static uint32_t allocate(s15_gui *s,uint32_t type,uint32_t role,int shared) {
 if(shared)for(unsigned i=0;i<CAP;i++)if(s->data.records[i].live&&s->data.records[i].type==type&&s->data.records[i].role==role)return s->data.records[i].id;
 unsigned i=0;while(i<CAP&&s->data.records[i].live)i++;if(i==CAP||s->data.next>=0xffffff)return 0;
 CFTypeRef object=object_for(s,type,role);if(!object)return 0;
 uint32_t id=0x52000000|s->data.next++;s->data.records[i]=(record){id,type,role,1};s->objects[i]=object;return id;
}
int s15_gui_call(s15_gui *s,uint32_t kind,uint32_t a,uint32_t b,uint32_t *value,uint32_t *error) {@autoreleasepool {
 (void)error;if(!NSThread.isMainThread||!unchanged(s))return 0;uint32_t *p=s->data.profile;
 if(kind==1) {switch(a) {case 0:*value=p[1];break;case 1:*value=p[2];break;case 11:*value=p[3];break;case 12:*value=p[4];break;case 2:case 3:*value=p[5];break;default:return 0;}return 1;}
 if(kind==2) {if(a)return 0;*value=allocate(s,1,0,0);return *value!=0;}
 if(kind==3) {int i=find(s,a);if(i<0||s->data.records[i].type!=1||(b!=88&&b!=90))return 0;*value=p[b==88?6:7];return 1;}
 if(kind==4) {int i=find(s,b);if(a||i<0||s->data.records[i].type!=1)return 0;CFRelease(s->objects[i]);s->objects[i]=NULL;s->data.records[i].live=0;*value=1;return 1;}
 if(kind==5) {int i=color_index(a);if(i<0)return 0;*value=p[13+i];return 1;}
 if(kind==6) {if(a!=15&&a!=6)return 0;*value=allocate(s,2,a,1);return *value!=0;}
 if(kind==7) {if(a||(b!=32512&&b!=32514))return 0;*value=allocate(s,3,b,1);return *value!=0;}
 return 0;
}}
int s15_gui_profile(s15_gui *s,uint32_t *p,size_t n) {if(n!=S15_PROFILE_WORDS)return 0;memcpy(p,s->data.profile,n*4);return 1;}
size_t s15_gui_size(void) {return sizeof(image);}
int s15_gui_export(s15_gui *s,void *p,size_t n) {if(n!=sizeof(image)||!unchanged(s))return 0;memcpy(p,&s->data,n);return 1;}
int s15_gui_import(s15_gui *s,const void *p,size_t n) {
 if(n!=sizeof(image)||!NSThread.isMainThread)return 0;image d;memcpy(&d,p,n);
 if(d.magic!=MAGIC||d.version!=VERSION||d.fixture!=s->data.fixture||!d.next||d.next>=0xffffff||memcmp(d.profile,s->data.profile,sizeof(d.profile))||!unchanged(s))return 0;
 CFTypeRef objects[CAP]={0};int okay=1;
 for(unsigned i=0;i<CAP&&okay;i++) {
  record r=d.records[i];if(r.live>1||r.type>3||(r.id&&((r.id&0xff000000)!=0x52000000||!(r.id&0xffffff)||(r.id&0xffffff)>=d.next))) {okay=0;break;}
  for(unsigned j=0;j<i;j++)if(r.id&&r.id==d.records[j].id)okay=0;
  if(r.live) {if(!r.id||!r.type)okay=0;else {objects[i]=object_for(s,r.type,r.role);if(!objects[i])okay=0;}}
 }
 if(!okay) {for(unsigned i=0;i<CAP;i++)if(objects[i])CFRelease(objects[i]);return 0;}
 for(unsigned i=0;i<CAP;i++)if(s->objects[i])CFRelease(s->objects[i]);memcpy(s->objects,objects,sizeof(objects));s->data=d;return 1;
}
uint32_t s15_gui_object_kind(s15_gui *s,uint32_t h) {
 int i=find(s,h);if(i<0||!s->objects[i])return 0;uint32_t type=s->data.records[i].type;
 if(type==1)return CFGetTypeID(s->objects[i])==(s->data.fixture?CFNumberGetTypeID():CGDisplayModeGetTypeID())?1:0;
 if(type==2)return CFGetTypeID(s->objects[i])==CGColorGetTypeID()?2:0;
 if(type==3)return [(__bridge id)s->objects[i] isKindOfClass:NSCursor.class]?3:0;return 0;
}
int s15_gui_brush_pixel(s15_gui *s,uint32_t h,uint32_t *out) {
 int i=find(s,h);if(i<0||s->data.records[i].type!=2)return 0;CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();unsigned char px[4]={0};
 CGContextRef c=CGBitmapContextCreate(px,1,1,8,4,space,kCGImageAlphaPremultipliedLast);CGColorSpaceRelease(space);if(!c)return 0;
 CGContextSetFillColorWithColor(c,(CGColorRef)s->objects[i]);CGContextFillRect(c,CGRectMake(0,0,1,1));CGContextRelease(c);*out=px[0]|((uint32_t)px[1]<<8)|((uint32_t)px[2]<<16);return 1;
}
