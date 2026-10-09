/* Platform surface backing only. No CPU, translated code, game data or EXE. */
#include "stage23_bridge.h"
#include <stdlib.h>
#include <string.h>
static Surface slots[64];
static int okay(Surface*s){return s&&s>=slots&&s<slots+64&&s->alive;}
int s23_initialize(void){return 1;}
uint64_t s23_native_offset(void){return 0;}
Surface*s23_create(int w,int h,int pitch,int f){
 int n=f==32?4:2;
 if((f!=32&&f!=555&&f!=565)||w<=0||h<=0||w>2048||h>2048||pitch<w*n||pitch%n)return NULL;
 for(int i=0;i<64;i++)if(!slots[i].alive){
  unsigned char*p=calloc((size_t)pitch,h);if(!p)return NULL;
  slots[i]=(Surface){.w=w,.h=h,.pitch=pitch,.fmt=f,.alive=1,.guest=(uint32_t)(i+1),.pixels=p};return slots+i;
 }return NULL;
}
int s23_free(Surface*s){if(!okay(s)||s->locked)return -1;free(s->pixels);memset(s,0,sizeof(*s));return 0;}
int s23_lock(Surface*s,uint32_t*g,int*p){if(!okay(s)||s->locked||!g||!p)return -1;s->locked=1;*g=s->guest;*p=s->pitch;return 0;}
int s23_unlock(Surface*s){if(!okay(s)||!s->locked)return -1;s->locked=0;return 0;}
int s23_format(Surface*s,uint32_t*m){if(!okay(s)||!m)return -1;m[0]=s->fmt==555?0x7c00:s->fmt==565?0xf800:0xff0000;m[1]=s->fmt==555?0x3e0:s->fmt==565?0x7e0:0xff00;m[2]=s->fmt==32?0xff:0x1f;m[3]=s->fmt==32?0xff000000:0;return 0;}
int s23_blt(Surface*d,int dx,int dy,Surface*s,S23Rect r,int keyed,uint32_t key){
 if(!okay(d)||!okay(s)||d->locked||s->locked||d->fmt!=32||keyed<0||keyed>1||r.w<0||r.h<0||r.w>8192||r.h>8192||r.x < -8192||r.x>8192||r.y < -8192||r.y>8192||dx < -8192||dx>8192||dy < -8192||dy>8192)return -1;
 int sx=r.x,sy=r.y,w=r.w,h=r.h;if(sx<0){dx-=sx;w+=sx;sx=0;}if(sy<0){dy-=sy;h+=sy;sy=0;}
 if(dx<0){sx-=dx;w+=dx;dx=0;}if(dy<0){sy-=dy;h+=dy;dy=0;}
 if(sx+w>s->w)w=s->w-sx;if(sy+h>s->h)h=s->h-sy;if(dx+w>d->w)w=d->w-dx;if(dy+h>d->h)h=d->h-dy;if(w<=0||h<=0)return 0;
 int b=s->fmt==32?4:2;unsigned char*copy=malloc((size_t)w*h*b);if(!copy)return -1;
 for(int y=0;y<h;y++)memcpy(copy+(size_t)y*w*b,s->pixels+(sy+y)*s->pitch+sx*b,(size_t)w*b);
 for(int y=0;y<h;y++)for(int x=0;x<w;x++){
  uint32_t value=0,rgba;memcpy(&value,copy+((size_t)y*w+x)*b,b);
  if(keyed&&value==key)continue;
  rgba=s->fmt==32?value:0xff000000u|(s->fmt==555?((value&0x7c00)<<9)|((value&0x3e0)<<6)|((value&0x1f)<<3):((value&0xf800)<<8)|((value&0x7e0)<<5)|((value&0x1f)<<3));
  memcpy(d->pixels+(dy+y)*d->pitch+(dx+x)*4,&rgba,4);
 }free(copy);return 0;
}
int s23_bltfast(Surface*d,int x,int y,Surface*s,S23Rect r,int k,uint32_t key){if(!okay(d)||!okay(s)||x<0||y<0||r.x<0||r.y<0||r.w<0||r.h<0||r.w>s->w||r.h>s->h||r.x>s->w-r.w||r.y>s->h-r.h||r.w>d->w||r.h>d->h||x>d->w-r.w||y>d->h-r.h)return -1;return s23_blt(d,x,y,s,r,k,key);}
