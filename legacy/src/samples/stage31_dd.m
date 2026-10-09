/* Native DD backing. Guest COM pointers stay in the Unicorn domain. The
 * already verified s23 backend executes original arm64 RGB conversion. */
#import <AppKit/AppKit.h>
#import <MetalKit/MetalKit.h>
#include "stage23_bridge.h"
#include <stdint.h>
#include <string.h>
enum{LIMIT=64};
typedef struct{uint32_t kind,refs,caps,hwnd,coop,clip,w,h,pitch,fmt,locked,reserved;}DObj;
typedef struct{uint32_t magic,version,w,h,fmt,modeowner,desktopw,desktoph,desktopfmt,reserved[3];DObj obj[LIMIT];}DImage;
@interface S31DD:NSObject<MTKViewDelegate>{
@public DImage im;Surface*surfaces[LIMIT];Surface*mode;__weak NSWindow*window;id<MTLDevice>gpu;id<MTLCommandQueue>queue;id<MTLTexture>texture;id<MTLRenderPipelineState>pipeline;MTKView*view;uint64_t frames;
}
@end
@implementation S31DD
-(void)mtkView:(MTKView*)v drawableSizeWillChange:(CGSize)size{(void)v;(void)size;}
-(void)drawInMTKView:(MTKView*)v{MTLRenderPassDescriptor*pass=v.currentRenderPassDescriptor;id<CAMetalDrawable>d=v.currentDrawable;if(!pass||!d||!texture)return;id<MTLCommandBuffer>cmd=[queue commandBuffer];id<MTLRenderCommandEncoder>enc=[cmd renderCommandEncoderWithDescriptor:pass];[enc setRenderPipelineState:pipeline];[enc setFragmentTexture:texture atIndex:0];[enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[enc endEncoding];[cmd presentDrawable:d];[cmd commit];[cmd waitUntilCompleted];}
-(void)dealloc{for(unsigned i=0;i<LIMIT;i++)if(surfaces[i]){if(surfaces[i]->locked)s23_unlock(surfaces[i]);s23_free(surfaces[i]);}if(mode)s23_free(mode);view.delegate=nil;}
@end
static S31DD*core(void*p){return(__bridge S31DD*)p;}
static DObj*obj(S31DD*c,uint32_t id){return id&&id<=LIMIT&&c->im.obj[id-1].refs?c->im.obj+id-1:NULL;}
static int fmt(uint32_t f){return f==555||f==565||f==32;}
void*s29_new(void*win,uint32_t w,uint32_t h){if(!win||!w||!h||!NSThread.isMainThread||!s23_initialize())return NULL;S31DD*c=[S31DD new];c->window=(__bridge NSWindow*)win;c->gpu=MTLCreateSystemDefaultDevice();c->queue=[c->gpu newCommandQueue];if(!c->gpu||!c->queue)return NULL;c->im.magic=0x44443239;c->im.version=1;c->im.w=c->im.desktopw=w;c->im.h=c->im.desktoph=h;c->im.fmt=c->im.desktopfmt=32;return(__bridge_retained void*)c;}
void s29_delete(void*p){if(p)CFRelease(p);}
uint32_t s29_create(void*p,uint32_t kind){S31DD*c=core(p);if(kind!=1&&kind!=3)return 0;for(unsigned i=0;i<LIMIT;i++)if(!c->im.obj[i].refs){c->im.obj[i]=(DObj){.kind=kind,.refs=1};return i+1;}return 0;}
uint32_t s29_addref(void*p,uint32_t id){DObj*o=obj(core(p),id);return o?++o->refs:0;}
uint32_t s29_release(void*p,uint32_t id){S31DD*c=core(p);DObj*o=obj(c,id);if(!o)return 0;if(o->refs==1&&o->locked)return 1;uint32_t n=--o->refs;if(!n){if(o->clip)s29_release(p,o->clip);if(c->surfaces[id-1]){s23_free(c->surfaces[id-1]);c->surfaces[id-1]=NULL;}if(c->im.modeowner==id){if(c->mode){s23_free(c->mode);c->mode=NULL;}c->im.modeowner=0;c->im.w=c->im.desktopw;c->im.h=c->im.desktoph;c->im.fmt=c->im.desktopfmt;}memset(o,0,sizeof(*o));}return n;}
int s29_coop(void*p,uint32_t id,uint32_t hwnd,uint32_t flags){S31DD*c=core(p);DObj*o=obj(c,id);if(!o||o->kind!=1||hwnd!=0x58000001||!c->window||(flags!=8&&flags!=19))return 0;o->hwnd=hwnd;o->coop=flags;return 1;}
int s29_mode(void*p,uint32_t id,uint32_t w,uint32_t h,uint32_t f){S31DD*c=core(p);DObj*o=obj(c,id);if(!o||o->kind!=1||o->coop!=19||!fmt(f)||!w||!h||w>2048||h>2048||(c->im.modeowner&&c->im.modeowner!=id))return 0;Surface*n=s23_create(w,h,w*(f==32?4:2),f);if(!n)return 0;MTLTextureDescriptor*td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:w height:h mipmapped:NO];td.usage=MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;__typeof__(c->texture) t=[c->gpu newTextureWithDescriptor:td];if(!t){s23_free(n);return 0;}if(c->mode)s23_free(c->mode);c->mode=n;c->texture=t;c->im.w=w;c->im.h=h;c->im.fmt=f;c->im.modeowner=id;return 1;}
int s29_display(void*p,uint32_t*out){S31DD*c=core(p);if(!out)return 0;out[0]=c->im.w;out[1]=c->im.h;out[2]=c->im.w*(c->im.fmt==32?4:2);out[3]=c->im.fmt;return 1;}
int s29_restore_mode(void*p,uint32_t id){S31DD*c=core(p);DObj*o=obj(c,id);if(!o||o->kind!=1||(c->im.modeowner&&c->im.modeowner!=id))return 0;if(c->mode){s23_free(c->mode);c->mode=NULL;}c->texture=nil;c->im.modeowner=0;c->im.w=c->im.desktopw;c->im.h=c->im.desktoph;c->im.fmt=c->im.desktopfmt;return 1;}
/* A virtual Win32 driver describes its actual BGRA bitmap/texture storage.
 * NSScreen.depth is recorded separately; it is not this driver's pixel stride. */
int s30_storage_report(void*p,uint32_t*out){
 S31DD*c=core(p);NSScreen*s=c->window.screen;if(!s||!out)return 0;
 unsigned char pixels[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16},back[16]={0};
 CGColorSpaceRef color=CGColorSpaceCreateDeviceRGB();CGContextRef b=CGBitmapContextCreate(pixels,2,2,8,8,color,kCGImageAlphaPremultipliedFirst|kCGBitmapByteOrder32Little);CGColorSpaceRelease(color);if(!b)return 0;
 MTLTextureDescriptor*desc=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:2 height:2 mipmapped:NO];desc.storageMode=MTLStorageModeShared;id<MTLTexture>t=[c->gpu newTextureWithDescriptor:desc];if(!t){CGContextRelease(b);return 0;}
 [t replaceRegion:MTLRegionMake2D(0,0,2,2) mipmapLevel:0 withBytes:pixels bytesPerRow:CGBitmapContextGetBytesPerRow(b)];[t getBytes:back bytesPerRow:8 fromRegion:MTLRegionMake2D(0,0,2,2) mipmapLevel:0];
 uint32_t a[10]={(uint32_t)s.depth,(uint32_t)NSBitsPerPixelFromDepth(s.depth),(uint32_t)NSBitsPerSampleFromDepth(s.depth),(uint32_t)CGBitmapContextGetBitsPerPixel(b),(uint32_t)CGBitmapContextGetBitsPerComponent(b),(uint32_t)CGBitmapContextGetBytesPerRow(b),(uint32_t)t.pixelFormat,(uint32_t)t.width,(uint32_t)t.height,memcmp(pixels,back,16)==0};memcpy(out,a,sizeof(a));CGContextRelease(b);return a[3]==32&&a[4]==8&&a[5]==8&&a[6]==MTLPixelFormatBGRA8Unorm&&a[9];
}
int s29_desktop_caps(void*p,uint32_t index,uint32_t*out){uint32_t report[10];if(!out||!s30_storage_report(p,report))return 0;if(index==24){*out=0;return 1;}if(index==12){*out=report[3];return 1;}if(index==14){*out=1;return 1;}return 0;}
uint32_t s29_surface(void*p,uint32_t dd,uint32_t w,uint32_t h,uint32_t pitch,uint32_t f,uint32_t caps){S31DD*c=core(p);DObj*d=obj(c,dd);if(!d||d->kind!=1||!d->hwnd||!fmt(f))return 0;Surface*s=s23_create(w,h,pitch,f);if(!s)return 0;for(unsigned i=0;i<LIMIT;i++)if(!c->im.obj[i].refs){c->im.obj[i]=(DObj){.kind=2,.refs=1,.caps=caps,.w=w,.h=h,.pitch=pitch,.fmt=f};c->surfaces[i]=s;return i+1;}s23_free(s);return 0;}
int s29_info(void*p,uint32_t id,uint32_t*out){DObj*o=obj(core(p),id);if(!o||!out)return 0;memcpy(out,o,sizeof(*o));return 1;}
size_t s29_pixel_size(void*p,uint32_t id){DObj*o=obj(core(p),id);return o&&o->kind==2?(size_t)o->pitch*o->h:0;}
int s29_pixels(void*p,uint32_t id,void*data,size_t n,uint32_t write){S31DD*c=core(p);DObj*o=obj(c,id);if(!o||o->kind!=2||!data||n!=s29_pixel_size(p,id)||write>1)return 0;Surface*s=c->surfaces[id-1];if(write)memcpy(s->pixels,data,n);else memcpy(data,s->pixels,n);return 1;}
int s29_lock(void*p,uint32_t id){S31DD*c=core(p);DObj*o=obj(c,id);uint32_t g;int pitch;if(!o||o->kind!=2||s23_lock(c->surfaces[id-1],&g,&pitch))return 0;o->locked=1;return 1;}
int s29_unlock(void*p,uint32_t id){S31DD*c=core(p);DObj*o=obj(c,id);if(!o||o->kind!=2||s23_unlock(c->surfaces[id-1]))return 0;o->locked=0;return 1;}
int s29_clipper_hwnd(void*p,uint32_t id,uint32_t hwnd){S31DD*c=core(p);DObj*o=obj(c,id);if(!o||o->kind!=3||hwnd!=0x58000001||!c->window)return 0;o->hwnd=hwnd;return 1;}
int s29_attach_clipper(void*p,uint32_t id,uint32_t clip){S31DD*c=core(p);DObj*o=obj(c,id),*cl=obj(c,clip);if(!o||o->kind!=2||(clip&&(!cl||cl->kind!=3)))return 0;if(clip)s29_addref(p,clip);if(o->clip)s29_release(p,o->clip);o->clip=clip;return 1;}
int s29_blt(void*p,uint32_t dst,int32_t dx,int32_t dy,uint32_t src,const int32_t*r){S31DD*c=core(p);DObj*d=obj(c,dst),*s=obj(c,src);if(!d||!s||d->kind!=2||s->kind!=2||!r)return 0;S23Rect q={r[0],r[1],r[2]-r[0],r[3]-r[1]};if(d->fmt==32)return s23_blt(c->surfaces[dst-1],dx,dy,c->surfaces[src-1],q,0,0)==0;if(d->locked||s->locked||d->fmt!=s->fmt||dx<0||dy<0||q.x<0||q.y<0||q.w<0||q.h<0||dx+q.w>(int)d->w||dy+q.h>(int)d->h||q.x+q.w>(int)s->w||q.y+q.h>(int)s->h)return 0;size_t row=(size_t)q.w*2,n=row*q.h;unsigned char*copy=malloc(n?n:1);if(!copy)return 0;for(int y=0;y<q.h;y++)memcpy(copy+y*row,c->surfaces[src-1]->pixels+(q.y+y)*s->pitch+q.x*2,row);for(int y=0;y<q.h;y++)memcpy(c->surfaces[dst-1]->pixels+(dy+y)*d->pitch+dx*2,copy+y*row,row);free(copy);return 1;}
size_t s29_size(void*p){S31DD*c=core(p);size_t n=sizeof(DImage);for(unsigned i=0;i<LIMIT;i++)if(c->im.obj[i].refs&&c->im.obj[i].kind==2)n+=(size_t)c->im.obj[i].pitch*c->im.obj[i].h;return n;}
int s29_export(void*p,void*data,size_t n){S31DD*c=core(p);if(!data||n!=s29_size(p))return 0;memcpy(data,&c->im,sizeof(c->im));unsigned char*out=(unsigned char*)data+sizeof(c->im);for(unsigned i=0;i<LIMIT;i++)if(c->surfaces[i]){size_t z=(size_t)c->im.obj[i].pitch*c->im.obj[i].h;memcpy(out,c->surfaces[i]->pixels,z);out+=z;}return 1;}
int s29_import(void*p,const void*data,size_t n){S31DD*c=core(p);if(!data||n<sizeof(DImage))return 0;DImage im;memcpy(&im,data,sizeof(im));if(im.magic!=0x44443239||im.version!=1||!fmt(im.fmt)||!im.desktopw||!im.desktoph||im.modeowner>LIMIT)return 0;size_t wanted=sizeof(im);for(unsigned i=0;i<LIMIT;i++){DObj*o=im.obj+i;if(o->refs){if(o->kind<1||o->kind>3||o->locked>1||o->reserved)return 0;if(o->kind==2){if(!fmt(o->fmt)||!o->w||!o->h||o->w>2048||o->h>2048||o->pitch<o->w*(o->fmt==32?4:2))return 0;wanted+=(size_t)o->pitch*o->h;}}}if(wanted!=n)return 0;for(unsigned i=0;i<LIMIT;i++)if(c->surfaces[i]){if(c->surfaces[i]->locked)s23_unlock(c->surfaces[i]);s23_free(c->surfaces[i]);c->surfaces[i]=NULL;}if(c->mode){s23_free(c->mode);c->mode=NULL;}const unsigned char*in=(const unsigned char*)data+sizeof(im);for(unsigned i=0;i<LIMIT;i++)if(im.obj[i].refs&&im.obj[i].kind==2){DObj*o=im.obj+i;Surface*s=s23_create(o->w,o->h,o->pitch,o->fmt);if(!s)return 0;c->surfaces[i]=s;size_t z=(size_t)o->pitch*o->h;memcpy(s->pixels,in,z);in+=z;if(o->locked){uint32_t g;int pitch;if(s23_lock(s,&g,&pitch))return 0;}}c->im=im;if(im.modeowner){c->mode=s23_create(im.w,im.h,im.w*(im.fmt==32?4:2),im.fmt);if(!c->mode)return 0;}return 1;}

int s31_fill(void*p,uint32_t id,uint32_t color,const int32_t*r,int32_t*out){
 S31DD*c=core(p);DObj*d=obj(c,id);if(!d||d->kind!=2||d->locked||!r||!out||r[2]<r[0]||r[3]<r[1])return 0;int l=MAX(r[0],0),t=MAX(r[1],0),right=MIN(r[2],(int)d->w),bottom=MIN(r[3],(int)d->h);
 if(d->clip){DObj*cl=obj(c,d->clip);if(!cl||cl->kind!=3||cl->hwnd!=0x58000001||!c->window)return 0;NSRect box=[c->window contentRectForFrameRect:c->window.frame];int x=(int)box.origin.x,y=(int)(NSMaxY(c->window.screen.frame)-NSMaxY(box));l=MAX(l,x);t=MAX(t,y);right=MIN(right,x+(int)box.size.width);bottom=MIN(bottom,y+(int)box.size.height);}
 if(right<l)right=l;if(bottom<t)bottom=t;Surface*s=c->surfaces[id-1];for(int y=t;y<bottom;y++){if(d->fmt==32){uint32_t*row=(uint32_t*)(s->pixels+y*d->pitch);for(int x=l;x<right;x++)row[x]=color;}else{uint16_t*row=(uint16_t*)(s->pixels+y*d->pitch);for(int x=l;x<right;x++)row[x]=(uint16_t)color;}}int32_t a[5]={l,t,right,bottom,(right-l)*(bottom-t)};memcpy(out,a,sizeof(a));return 1;
}
