/* Actual owned-window cursor rectangles. Never calls NSCursor.set/hide/warp. */
#import <AppKit/AppKit.h>
#include <stdint.h>
#include <string.h>
typedef struct{uint32_t id,kind,x,y,n,pad;unsigned char data[8192];} Resource43;
typedef struct{uint32_t atom,words[9];char name[128];} Class43;
typedef struct{uint32_t handle,tid,creating,style,exstyle;int32_t x,y,w,h;char title[256],class_name[128];} Window43;
typedef struct{uint32_t magic,version,next,pad;Resource43 resources[8];Class43 cls;Window43 window;} SourceImage43;
typedef struct{SourceImage43 im;__unsafe_unretained id objects[8];} SourceCore43;
_Static_assert(sizeof(SourceImage43)==66332,"fixed reviewed stage31 image ABI");
typedef struct{uint32_t magic,version,selected,n;double hot_x,hot_y;} Header43;
@interface Cursor43:NSObject
@property(nonatomic)void*source;
@property(nonatomic,strong)NSWindow*window;
@property(nonatomic,strong)NSCursor*previousNative;
@property(nonatomic,strong)NSData*previousImage;
@property(nonatomic)Header43 im;
@end
@implementation Cursor43
@end
static NSCursor*lookup(Cursor43*s,uint32_t h){if(h==0x5a000001)return s.previousNative;SourceCore43*c=s.source;if(!c||c->im.magic!=0x57494e32||c->im.version!=1)return nil;for(int i=0;i<8;i++)if(c->im.resources[i].id==h&&c->im.resources[i].kind==3&&[c->objects[i]isKindOfClass:NSCursor.class])return c->objects[i];return nil;}
static int install(Cursor43*s,uint32_t h){NSCursor*c=lookup(s,h);if(!c||!NSThread.isMainThread||!s.window.contentView)return 0;BOOL active=NSApp.isActive,key=s.window.isKeyWindow;NSCursor*globalBefore=NSCursor.currentCursor;[s.window.contentView discardCursorRects];[s.window.contentView addCursorRect:s.window.contentView.bounds cursor:c];return active==NSApp.isActive&&key==s.window.isKeyWindow&&globalBefore==NSCursor.currentCursor;}
void*s43c_new(void*source,void*window){if(!source||!window||!NSThread.isMainThread)return NULL;Cursor43*s=[Cursor43 new];s.source=source;s.window=(__bridge NSWindow*)window;s.previousNative=NSCursor.currentCursor;if(!s.previousNative)return NULL;s.previousImage=s.previousNative.image.TIFFRepresentation;if(!s.previousImage.length||s.previousImage.length>1048576)return NULL;Header43 im={0x53343343,1,0x5a000001,(uint32_t)s.previousImage.length,s.previousNative.hotSpot.x,s.previousNative.hotSpot.y};s.im=im;return (__bridge_retained void*)s;}
int s43c_select(void*p,uint32_t h,uint32_t*out){Cursor43*s=(__bridge Cursor43*)p;if(!s||!out||!lookup(s,h))return 0;Header43 im=s.im;*out=im.selected;if(h!=im.selected&&!install(s,h))return 0;im.selected=h;s.im=im;return 1;}
size_t s43c_size(void*p){Cursor43*s=(__bridge Cursor43*)p;return s?sizeof(Header43)+s.previousImage.length:0;}
int s43c_export(void*p,void*out,size_t n){Cursor43*s=(__bridge Cursor43*)p;if(!s||!out||n!=s43c_size(p))return 0;Header43 im=s.im;memcpy(out,&im,sizeof(im));memcpy((char*)out+sizeof(im),s.previousImage.bytes,im.n);return 1;}
int s43c_import(void*p,const void*in,size_t n){Cursor43*s=(__bridge Cursor43*)p;if(!s||!in||n<sizeof(Header43))return 0;Header43 im;memcpy(&im,in,sizeof(im));if(im.magic!=0x53343343||im.version!=1||!im.n||im.n>1048576||n!=sizeof(im)+im.n)return 0;NSData*d=[NSData dataWithBytes:(const char*)in+sizeof(im)length:im.n];NSImage*i=[[NSImage alloc]initWithData:d];if(!i)return 0;s.previousNative=[[NSCursor alloc]initWithImage:i hotSpot:NSMakePoint(im.hot_x,im.hot_y)];s.previousImage=d;if(!lookup(s,im.selected)||(im.selected!=0x5a000001&&!install(s,im.selected)))return 0;s.im=im;return 1;}
void s43c_delete(void*p){if(!p)return;Cursor43*s=CFBridgingRelease(p);[s.window.contentView discardCursorRects];}
