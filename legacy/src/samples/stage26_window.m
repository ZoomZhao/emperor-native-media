/* Real AppKit objects plus a task-local Win32 class/handle namespace. */
#import <AppKit/AppKit.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
enum{CAP=8,DATA=8192};
typedef struct{uint32_t id,kind,x,y,n,pad;unsigned char data[DATA];}Resource;
typedef struct{uint32_t atom,words[9];char name[128];}GuestClass;
typedef struct{uint32_t handle,tid,creating,style,exstyle;int32_t x,y,w,h;char title[256],class_name[128];}Window;
typedef struct{uint32_t magic,version,next,pad;Resource resources[CAP];GuestClass cls;Window window;}Image;
typedef struct{Image im;id objects[CAP];NSWindow*window;}Core;
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
static NSWindow*make_window(Window*w){
 if(!NSThread.isMainThread||w->w<=0||w->h<=0||w->style!=0x90080000u||w->exstyle!=0x40000u)return nil;
 NSScreen*s=NSScreen.mainScreen;if(!s)return nil;NSRect frame=NSMakeRect(w->x,s.frame.size.height-w->y-w->h,w->w,w->h);
 NSWindow*win=[[NSWindow alloc]initWithContentRect:frame styleMask:NSWindowStyleMaskBorderless backing:NSBackingStoreBuffered defer:NO];win.releasedWhenClosed=NO;win.title=[NSString stringWithUTF8String:w->title];win.backgroundColor=NSColor.blackColor;return win;
}
void*s26_window_new(void){if(!NSThread.isMainThread)return NULL;[NSApplication sharedApplication];Core*c=calloc(1,sizeof(*c));if(!c)return NULL;c->im.magic=0x57494e32;c->im.version=1;c->im.next=1;return c;}
void s26_window_delete(Core*c){if(c){for(unsigned i=0;i<CAP;i++)[c->objects[i]release];[c->window close];[c->window release];free(c);}}
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
 if(okay&&im->window.handle){if(im->window.handle!=0x58000001||im->window.creating!=1||!memchr(im->window.title,0,256)||!memchr(im->window.class_name,0,128))okay=0;else{win=make_window(&im->window);if(!win)okay=0;}}
 if(!okay){for(unsigned i=0;i<CAP;i++)[objects[i]release];[win close];[win release];free(im);return 0;}
 for(unsigned i=0;i<CAP;i++)[c->objects[i]release];[c->window close];[c->window release];memcpy(c->objects,objects,sizeof(objects));c->window=win;c->im=*im;free(im);return 1;
}
