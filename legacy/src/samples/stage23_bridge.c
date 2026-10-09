/* Native bounded DirectDraw semantic subset. Original conversion is LLVM arm64. */
#include "stage23_bridge.h"
#include "native_cpu.h"
#include <stdio.h>
#include <stdlib.h>
extern void c_convert555(CPU*);extern void c_convert565(CPU*);
static _Alignas(16) unsigned char arena[16*1024*1024];
typedef struct{size_t offset,size;int available;} Block;static Block blocks[128];static unsigned blockCount;
static size_t used;static Surface slots[64];static Surface *frames[8];static unsigned char*initials[8];
const unsigned char*s23_frame_initial(unsigned i){return i<8?initials[i]:NULL;}
Surface*s23_frame(unsigned i){return i<8?frames[i]:NULL;}static CPU cpu;static unsigned long original_calls;
int s23_initialize(void){return cpu.esp?1:initialize_cpu(&cpu,arena);}
uint64_t s23_native_offset(void){return cpu.pointer_offset;}
static int ok(Surface*s){return s && s>=slots && s<slots+64 && s->alive;}
Surface*s23_create(int w,int h,int pitch,int fmt){
 int bytes=fmt==32?4:2;if((fmt!=32&&fmt!=555&&fmt!=565)||w<=0||h<=0||w>2048||h>2048||pitch<w*bytes||pitch%bytes)return NULL;
 size_t n=(size_t)pitch*h;int slot=-1;for(int i=0;i<64;i++)if(!slots[i].alive){slot=i;break;}if(slot<0)return NULL;
 int block=-1;for(unsigned i=0;i<blockCount;i++)if(blocks[i].available&&blocks[i].size>=n){block=(int)i;break;}
 if(block<0){if(blockCount==128||n>sizeof(arena)-used-16)return NULL;block=(int)blockCount++;used=(used+15)&~(size_t)15;blocks[block].offset=used;blocks[block].size=n;used+=n;}blocks[block].available=0;
 {Surface*s=&slots[slot];s->block=block;s->pixels=arena+blocks[block].offset;s->w=w;s->h=h;s->pitch=pitch;s->fmt=fmt;s->alive=1;s->locked=0;s->guest=pointer32(&cpu,s->pixels);if(!s->guest){s->alive=0;return NULL;}memset(s->pixels,0,n);return s;}return NULL;
}
int s23_free(Surface*s){if(!ok(s)||s->locked)return -1;blocks[s->block].available=1;s->alive=0;return 0;}
int s23_lock(Surface*s,uint32_t*g,int*p){if(!ok(s)||s->locked||!g||!p)return -1;s->locked=1;*g=s->guest;*p=s->pitch;return 0;}
int s23_unlock(Surface*s){if(!ok(s)||!s->locked)return -1;s->locked=0;return 0;}
int s23_format(Surface*s,uint32_t*m){if(!ok(s)||!m)return -1;m[0]=s->fmt==555?0x7c00:s->fmt==565?0xf800:0xff0000;m[1]=s->fmt==555?0x3e0:s->fmt==565?0x7e0:0xff00;m[2]=s->fmt==32?0xff:0x1f;m[3]=s->fmt==32?0xff000000:0;return 0;}
static int convertrow(Surface*s,int x,int y,Surface*t,int n){
 uint32_t sp=cpu.esp,args[8]={pointer32(&cpu,s->pixels+y*s->pitch+x*2),t->guest,(uint32_t)n,1,(uint32_t)t->pitch/4,0,0,0};
 cpu.ebx=0x12345678;cpu.esi=0x23456789;cpu.edi=0x3456789a;cpu.ebp=0x456789ab;
 for(int i=7;i>=0;i--){cpu.esp-=4;memcpy((void*)(cpu.pointer_offset+cpu.esp),args+i,4);}
 if(s->fmt==555)c_convert555(&cpu);else c_convert565(&cpu);cpu.esp+=32;original_calls++;
 return cpu.esp==sp&&cpu.ebx==0x12345678&&cpu.esi==0x23456789&&cpu.edi==0x3456789a&&cpu.ebp==0x456789ab?0:-1;
}
int s23_blt(Surface*d,int dx,int dy,Surface*s,S23Rect r,int keyed,uint32_t key){
 if(!ok(d)||!ok(s)||d->locked||s->locked||d->fmt!=32||(s->fmt!=32&&s->fmt!=555&&s->fmt!=565)||keyed<0||keyed>1||r.w<0||r.h<0||r.x < -8192||r.x>8192||r.y < -8192||r.y>8192||r.w>8192||r.h>8192||dx < -8192||dx>8192||dy < -8192||dy>8192)return -1;
 if(!r.w||!r.h)return 0;int sx=r.x,sy=r.y,w=r.w,h=r.h;
 if(sx<0){dx-=sx;w+=sx;sx=0;}if(sy<0){dy-=sy;h+=sy;sy=0;}
 if(dx<0){sx-=dx;w+=dx;dx=0;}if(dy<0){sy-=dy;h+=dy;dy=0;}
 if(sx+w>s->w)w=s->w-sx;if(sy+h>s->h)h=s->h-sy;if(dx+w>d->w)w=d->w-dx;if(dy+h>d->h)h=d->h-dy;if(w<=0||h<=0)return 0;
 Surface*t=s23_create(w,1,w*4,32);if(!t)return -1;
 /* Snapshot protects same-surface overlapping rectangles and persistent rows. */
 unsigned char*snap=NULL;if(s==d){snap=malloc((size_t)w*h*4);if(!snap){s23_free(t);return -1;}for(int y=0;y<h;y++)memcpy(snap+y*w*4,s->pixels+(sy+y)*s->pitch+sx*4,w*4);}
 for(int y=0;y<h;y++){
  if(s->fmt==32)memcpy(t->pixels,snap?snap+y*w*4:s->pixels+(sy+y)*s->pitch+sx*4,w*4);
  else if(convertrow(s,sx,sy+y,t,w)){free(snap);s23_free(t);return -1;}
  for(int x=0;x<w;x++){uint32_t value=0;if(s->fmt==32)memcpy(&value,t->pixels+x*4,4);else{uint16_t v;memcpy(&v,s->pixels+(sy+y)*s->pitch+(sx+x)*2,2);value=v;}if(!keyed||value!=key)memcpy(d->pixels+(dy+y)*d->pitch+(dx+x)*4,t->pixels+x*4,4);}
 }
 free(snap);return s23_free(t);
}
int s23_bltfast(Surface*d,int x,int y,Surface*s,S23Rect r,int k,uint32_t key){if(!ok(d)||!ok(s)||x<0||y<0||r.x<0||r.y<0||r.w<0||r.h<0||r.w>s->w||r.h>s->h||r.x>s->w-r.w||r.y>s->h-r.h||r.w>d->w||r.h>d->h||x>d->w-r.w||y>d->h-r.h)return -1;return s23_blt(d,x,y,s,r,k,key);}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"bridge verification failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int s23_verify(const char*path,const unsigned char**image,int*width,int*height,int*pitch){
 CHECK(initialize_cpu(&cpu,arena));FILE*f=fopen(path,"rb");CHECK(f);uint32_t count;CHECK(fread(&count,4,1,f)==1&&count==8);int checks=0;
 for(uint32_t i=0;i<count;i++){int32_t a[12];CHECK(fread(a,sizeof(a),1,f)==1);Surface*s=s23_create(a[1],a[2],a[3],a[0]),*d=s23_create(a[4],a[5],a[6],32);CHECK(s&&d);CHECK(fread(s->pixels,a[11],1,f)==1);size_t n=(size_t)a[6]*a[5];unsigned char*ex=malloc(n);CHECK(ex&&fread(ex,n,1,f)==1);memset(d->pixels,0xa5,n);
  uint32_t gp,m[4];int p;CHECK(!s23_lock(s,&gp,&p)&&gp==s->guest&&p==a[3]);CHECK(s23_lock(s,&gp,&p)==-1&&s23_free(s)==-1);CHECK(!s23_unlock(s)&&s23_unlock(s)==-1);CHECK(!s23_format(s,m)&&m[0]==(a[0]==555?0x7c00:0xf800)&&m[1]==(a[0]==555?0x3e0:0x7e0)&&m[2]==0x1f&&m[3]==0);checks+=5;
  S23Rect all={0,0,a[1],a[2]};CHECK(!s23_blt(d,a[7],a[8],s,all,a[9],a[10]));CHECK(!memcmp(d->pixels,ex,n));checks++;
  int fastFits=a[7]>=0&&a[8]>=0&&a[7]+a[1]<=d->w&&a[8]+a[2]<=d->h;CHECK(s23_bltfast(d,a[7],a[8],s,all,a[9],a[10])==(fastFits?0:-1)&&!memcmp(d->pixels,ex,n));checks++;
  /* Split rows: genuine partial updates persist without clearing destination. */
  memset(d->pixels,0xa5,n);S23Rect top={0,0,a[1],a[2]/2},bottom={0,a[2]/2,a[1],a[2]-a[2]/2};CHECK(!s23_blt(d,a[7],a[8],s,top,a[9],a[10]));CHECK(!s23_blt(d,a[7],a[8]+bottom.y,s,bottom,a[9],a[10]));CHECK(!memcmp(d->pixels,ex,n));checks++;
  CHECK(s23_blt(d,0,0,s,all,2,0)==-1);checks++;
  initials[i]=malloc(n);CHECK(initials[i]);memcpy(initials[i],d->pixels,n);CHECK(fread(ex,n,1,f)==1);int mx=a[7]<0?-a[7]:0,my=a[8]<0?-a[8]:0;
  if(mx<s->w&&my<s->h&&a[7]+mx>=0&&a[7]+mx<d->w&&a[8]+my>=0&&a[8]+my<d->h){CHECK(!s23_lock(s,&gp,&p));unsigned char*locked=(unsigned char*)(cpu.pointer_offset+gp);uint16_t v;memcpy(&v,locked+my*p+mx*2,2);v^=0x1f;memcpy(locked+my*p+mx*2,&v,2);CHECK(!s23_unlock(s));S23Rect changed={mx,my,1,1};CHECK(!s23_blt(d,a[7]+mx,a[8]+my,s,changed,a[9],a[10]));}CHECK(!memcmp(d->pixels,ex,n));checks++;

  frames[i]=d;if(i==0){*image=d->pixels;*width=d->w;*height=d->h;*pitch=d->pitch;}CHECK(!s23_free(s)&&s23_lock(s,&gp,&p)==-1);free(ex);
 }
 CHECK(fgetc(f)==EOF);fclose(f);CHECK(!s23_create(0,1,4,32)&&!s23_create(2,2,3,555)&&!s23_create(2,2,8,999));
 Surface*copy=s23_create(4,3,24,32);CHECK(copy);for(int y=0;y<3;y++)for(int x=0;x<4;x++){uint32_t value=(uint32_t)(y*4+x+1);memcpy(copy->pixels+y*24+x*4,&value,4);}S23Rect overlap={0,0,3,2};CHECK(!s23_bltfast(copy,1,1,copy,overlap,0,0));uint32_t desired[12]={1,2,3,4,5,1,2,3,9,5,6,7};for(int y=0;y<3;y++)for(int x=0;x<4;x++){uint32_t got;memcpy(&got,copy->pixels+y*24+x*4,4);CHECK(got==desired[y*4+x]);}CHECK(!s23_free(copy));checks+=14;
 for(int i=0;i<100;i++){Surface*reuse=s23_create(7,5,32,32);CHECK(reuse&&s23_free(reuse)==0);}checks++;
 printf("PASS native-original conversion: %u cases, %d checks, %lu original arm64 calls; full pixels and row padding equal original-byte oracle\n",count,checks+3,original_calls);
 return 0;
}
