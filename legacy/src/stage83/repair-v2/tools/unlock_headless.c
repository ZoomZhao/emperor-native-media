#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
typedef struct {int w,h,pitch,fmt,alive,locked,block;uint32_t guest;unsigned char *pixels;} Surface;
enum{LIMIT=64};
typedef struct{uint32_t kind,refs,caps,hwnd,coop,clip,w,h,pitch,fmt,locked,reserved;}DObj;
typedef struct{uint32_t magic,version,w,h,fmt,modeowner,desktopw,desktoph,desktopfmt,reserved[3];DObj obj[LIMIT];}DImage;
typedef struct{DImage im;Surface*surfaces[LIMIT];}S30DD;
static Surface slots[64];
static S30DD*core(void*p){return p;}
static int ok(Surface*s){return s && s>=slots && s<slots+64 && s->alive;}
int s23_unlock(Surface*s){if(!ok(s)||!s->locked)return -1;s->locked=0;return 0;}
static DObj*obj(S30DD*c,uint32_t id){return id&&id<=LIMIT&&c->im.obj[id-1].refs?c->im.obj+id-1:NULL;}
int s29_unlock(void*p,uint32_t id){S30DD*c=core(p);DObj*o=obj(c,id);if(!o||o->kind!=2||s23_unlock(c->surfaces[id-1]))return 0;o->locked=0;return 1;}

void s84_delete(void*p){if(!p)return;S30DD*c=p;for(unsigned i=0;i<64;i++)if(c->surfaces[i]){free(c->surfaces[i]->pixels);memset(c->surfaces[i],0,sizeof(Surface));}free(c);}
void*s84_import(const void*b,size_t n){
 if(!b||n<sizeof(DImage))return NULL;const DImage*im=b;
 if(im->magic!=0x44443239||im->version!=1||im->modeowner>64)return NULL;
 size_t total=sizeof(*im);for(unsigned i=0;i<64;i++){const DObj*o=im->obj+i;if(!o->refs)continue;if(o->kind<1||o->kind>3||o->reserved||o->locked>1)return NULL;if(o->kind==2){if(!o->w||!o->h||o->w>2048||o->h>2048||(o->fmt!=32&&o->fmt!=555&&o->fmt!=565)||o->pitch<o->w*(o->fmt==32?4:2))return NULL;total+=(size_t)o->pitch*o->h;}}
 if(total!=n)return NULL;for(unsigned i=0;i<64;i++)if(slots[i].alive)return NULL;
 S30DD*c=calloc(1,sizeof(*c));if(!c)return NULL;c->im=*im;const unsigned char*in=(const unsigned char*)b+sizeof(DImage);
 for(unsigned i=0;i<64;i++){DObj*o=c->im.obj+i;if(o->refs&&o->kind==2){Surface*s=slots+i;size_t z=(size_t)o->pitch*o->h;s->pixels=malloc(z);if(!s->pixels){s84_delete(c);return NULL;}memcpy(s->pixels,in,z);in+=z;s->w=o->w;s->h=o->h;s->pitch=o->pitch;s->fmt=o->fmt;s->alive=1;s->locked=o->locked;c->surfaces[i]=s;}}
 return c;
}
int s84_sync_unlock(void*p,uint32_t id,const void*bytes,size_t n){S30DD*c=p;if(!c)return 0;DObj*o=obj(c,id);if(!o||o->kind!=2||!bytes||n!=(size_t)o->pitch*o->h)return 0;memcpy(c->surfaces[id-1]->pixels,bytes,n);return s29_unlock(c,id);}
int s84_export(void*p,void*b,size_t n){S30DD*c=p;if(!c||!b||n<sizeof(DImage))return 0;size_t total=sizeof(DImage);for(unsigned i=0;i<64;i++)if(c->surfaces[i])total+=(size_t)c->im.obj[i].pitch*c->im.obj[i].h;if(total!=n)return 0;memcpy(b,&c->im,sizeof(DImage));unsigned char*out=(unsigned char*)b+sizeof(DImage);for(unsigned i=0;i<64;i++)if(c->surfaces[i]){size_t z=(size_t)c->im.obj[i].pitch*c->im.obj[i].h;memcpy(out,c->surfaces[i]->pixels,z);out+=z;}return 1;}
