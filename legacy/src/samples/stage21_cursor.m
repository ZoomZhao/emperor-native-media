#import <AppKit/AppKit.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef struct {uint32_t id,live,x,y;uint8_t a[128],b[128];} rec;
typedef struct {uint32_t magic,version,next,pad;rec r[8];} image;
typedef struct {image im;NSCursor *objects[8];} core;
static NSCursor *make(rec *r){
 if(r->x==32513)return [NSCursor.IBeamCursor retain];
 if(r->x==32644)return [NSCursor.resizeLeftRightCursor retain];
 if(r->x==32645)return [NSCursor.resizeUpDownCursor retain];
 if(r->x==32646)return [NSCursor.openHandCursor retain];
 NSBitmapImageRep *rep=[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL pixelsWide:32 pixelsHigh:32 bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:128 bitsPerPixel:32];
 if(!rep)return nil;unsigned char *p=rep.bitmapData;
 for(int i=0;i<1024;i++){int a=(r->a[i/8]>>(7-i%8))&1,b=(r->b[i/8]>>(7-i%8))&1;if(a&&b){[rep release];return nil;}p[4*i]=p[4*i+1]=p[4*i+2]=(!a&&b)?255:0;p[4*i+3]=a?0:255;}
 NSImage *im=[[NSImage alloc] initWithSize:NSMakeSize(32,32)];[im addRepresentation:rep];NSCursor *c=[[NSCursor alloc] initWithImage:im hotSpot:NSMakePoint(r->x,r->y)];[im release];[rep release];return c;
}
void *s21_new(void){core *c=calloc(1,sizeof(core));c->im.magic=0x43555232;c->im.version=1;c->im.next=1;return c;}
void s21_delete(core *c){if(!c)return;for(int i=0;i<8;i++)[c->objects[i] release];free(c);}
uint32_t s21_create(core *c,uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint8_t *a,const uint8_t *b){if(x>=32||y>=32||w!=32||h!=32||c->im.next>8)return 0;int i=c->im.next-1;rec r={0};r.id=0x55000000|c->im.next;r.live=1;r.x=x;r.y=y;memcpy(r.a,a,128);memcpy(r.b,b,128);NSCursor *n=make(&r);if(!n)return 0;c->objects[i]=n;c->im.r[i]=r;c->im.next++;return r.id;}
uint32_t s21_load(core *c,uint32_t role){if(role!=32513&&role!=32644&&role!=32645&&role!=32646)return 0;for(int i=0;i<8;i++)if(c->im.r[i].live&&c->im.r[i].x==role)return c->im.r[i].id;if(c->im.next>8)return 0;int i=c->im.next-1;rec r={0};r.id=0x55000000|c->im.next;r.live=1;r.x=role;NSCursor *n=make(&r);if(!n)return 0;c->im.r[i]=r;c->objects[i]=n;c->im.next++;return r.id;}
int s21_destroy(core *c,uint32_t h){for(int i=0;i<8;i++)if(c->im.r[i].id==h&&c->im.r[i].live){[c->objects[i] release];c->objects[i]=nil;c->im.r[i].live=0;return 1;}return 0;}
size_t s21_size(void){return sizeof(image);}
int s21_export(core *c,void *p,size_t n){if(n!=sizeof(image))return 0;memcpy(p,&c->im,n);return 1;}
int s21_import(core *c,const void *p,size_t n){if(n!=sizeof(image))return 0;image im;memcpy(&im,p,n);if(im.magic!=0x43555232||im.version!=1||im.next<1||im.next>9)return 0;NSCursor *v[8]={0};for(int i=0;i<8;i++)if(im.r[i].live){uint32_t x=im.r[i].x;int system=x==32513||x==32644||x==32645||x==32646;if(im.r[i].id!=(0x55000001u+i)||(!system&&x>=32)||im.r[i].y>=32)goto fail;v[i]=make(&im.r[i]);if(!v[i])goto fail;}for(int i=0;i<8;i++){[c->objects[i] release];c->objects[i]=v[i];}c->im=im;return 1;fail:for(int i=0;i<8;i++)[v[i] release];return 0;}
int s21_pixels(core *c,uint32_t h,uint8_t *p,size_t n,uint32_t *xy){if(n!=4096)return 0;for(int i=0;i<8;i++)if(c->im.r[i].id==h&&c->im.r[i].live){NSCursor *cur=c->objects[i];NSBitmapImageRep *r=(NSBitmapImageRep *)cur.image.representations[0];if(![r isKindOfClass:[NSBitmapImageRep class]]||r.bytesPerRow!=128||r.pixelsWide!=32||r.pixelsHigh!=32)return 0;memcpy(p,r.bitmapData,n);xy[0]=(uint32_t)cur.hotSpot.x;xy[1]=(uint32_t)cur.hotSpot.y;return 1;}return 0;}
