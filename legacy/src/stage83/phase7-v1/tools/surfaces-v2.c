#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
typedef struct {int w,h,pitch,fmt,alive,locked,block;uint32_t guest;unsigned char *pixels;} Surface;
enum{LIMIT=64};
typedef struct{uint32_t kind,refs,caps,hwnd,coop,clip,w,h,pitch,fmt,locked,reserved;}DObj;
typedef struct{uint32_t magic,version,w,h,fmt,modeowner,desktopw,desktoph,desktopfmt,reserved[3];DObj obj[LIMIT];}DImage;
typedef struct{DImage im;Surface*surfaces[LIMIT];void*gpu;}S30DD;
static Surface slots[64];
static S30DD*core(void*p){return p;}
static int ok(Surface*s){return s && s>=slots && s<slots+64 && s->alive;}
int s23_unlock(Surface*s){if(!ok(s)||!s->locked)return -1;s->locked=0;return 0;}
static DObj*obj(S30DD*c,uint32_t id){return id&&id<=LIMIT&&c->im.obj[id-1].refs?c->im.obj+id-1:NULL;}
int s23_lock(Surface*s,uint32_t*g,int*p){if(!ok(s)||s->locked||!g||!p)return -1;s->locked=1;*g=s->guest;*p=s->pitch;return 0;}
int s29_lock(void*p,uint32_t id){S30DD*c=core(p);DObj*o=obj(c,id);uint32_t g;int pitch;if(!o||o->kind!=2||s23_lock(c->surfaces[id-1],&g,&pitch))return 0;o->locked=1;return 1;}
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

int p7_sync(void*p,uint32_t id,const void*bytes,size_t n){S30DD*c=p;DObj*o=c?obj(c,id):NULL;if(!o||o->kind!=2||!bytes||n!=(size_t)o->pitch*o->h)return 0;memcpy(c->surfaces[id-1]->pixels,bytes,n);return 1;}
int p7_lock(void*p,uint32_t id,uint32_t guest){S30DD*c=p;DObj*o=c?obj(c,id):NULL;if(!o||o->kind!=2)return 0;c->surfaces[id-1]->guest=guest;return s29_lock(p,id);}
/* The reviewed s23 32-bit path, including clip order and overlap snapshot.
 * 16-bit conversion requires original SR code and is refused before any store. */
int p7_blt32(void*p,uint32_t dst,int dx,int dy,uint32_t src,const int32_t*r){
 S30DD*c=p;DObj*d=c?obj(c,dst):NULL,*s=c?obj(c,src):NULL;
 if(!d||!s||d->kind!=2||s->kind!=2||!r||d->locked||s->locked||d->fmt!=32||s->fmt!=32)return 0;
 int64_t rw=(int64_t)r[2]-r[0],rh=(int64_t)r[3]-r[1];
 if(rw<0||rh<0||rw>8192||rh>8192||r[0]<-8192||r[0]>8192||r[1]<-8192||r[1]>8192||dx<-8192||dx>8192||dy<-8192||dy>8192)return 0;
 int sx=r[0],sy=r[1],w=(int)rw,h=(int)rh;if(!w||!h)return 1;
 if(sx<0){dx-=sx;w+=sx;sx=0;}if(sy<0){dy-=sy;h+=sy;sy=0;}
 if(dx<0){sx-=dx;w+=dx;dx=0;}if(dy<0){sy-=dy;h+=dy;dy=0;}
 if(sx+w>(int)s->w)w=(int)s->w-sx;if(sy+h>(int)s->h)h=(int)s->h-sy;
 if(dx+w>(int)d->w)w=(int)d->w-dx;if(dy+h>(int)d->h)h=(int)d->h-dy;if(w<=0||h<=0)return 1;
 size_t row=(size_t)w*4,n=row*h;unsigned char*copy=malloc(n);if(!copy)return 0;
 for(int y=0;y<h;y++)memcpy(copy+y*row,c->surfaces[src-1]->pixels+(sy+y)*s->pitch+sx*4,row);
 for(int y=0;y<h;y++)memcpy(c->surfaces[dst-1]->pixels+(dy+y)*d->pitch+dx*4,copy+y*row,row);
 free(copy);return 1;
}

typedef S30DD S34DD;
#define nil NULL
int s34_is_lost(void*p,uint32_t surface_id,uint32_t*out){S34DD*c=core(p);DObj*d=obj(c,surface_id);if(!d||d->kind!=2||!out)return 0;Surface*s=c->surfaces[surface_id-1];uint32_t resident=s&&s->alive&&s->pixels&&s->w==(int)d->w&&s->h==(int)d->h&&s->pitch==(int)d->pitch&&s->fmt==(int)d->fmt;uint32_t a[7]={resident,s?s->alive:0,s?s->w:0,s?s->h:0,s?s->pitch:0,s?s->locked:0,(uint32_t)(c->gpu!=nil)};memcpy(out,a,sizeof(a));return 1;}
