/* Native platform contracts; all sprite, clipping and refresh logic is original LLVM. */
#include "stage24_bridge.h"
#include "native_cpu.h"
#include "stage24_generated.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define FAIL UINT32_C(0x80004005)
#define INVALID UINT32_C(0x80070057)
#define NOIMPL UINT32_C(0x80004001)
typedef struct{uint32_t vt,refs,kind,clipper;Surface*surface;uint32_t caps,hwnd;} Obj;
static CPU cpu;static Obj objects[128];static uint32_t vtdd[23],vts[36],vtclip[9];static void*bound_window;
static _Alignas(16) unsigned char manager[4096+52292*41],records[172*72],pixels[2100000];
static uint32_t descriptor[27],output_object;static unsigned count;
static Surface*frames[16];static unsigned char*initials[16];static unsigned long original_calls,com_calls[20];
void stage24_bind_window(void*p){bound_window=p;}
Surface*stage24_frame(unsigned i){return i<count?frames[i]:NULL;}
const unsigned char*stage24_initial(unsigned i){return i<count?initials[i]:NULL;}
unsigned stage24_frame_count(void){return count;}
static void stop(const char*what){fprintf(stderr,"stage24 contract failure: %s\n",what);exit(70);}
static void*p32(uint32_t a){if(!a)stop("null guest pointer");return(void*)(cpu.pointer_offset+a);}
static uint32_t gp(const void*p){uint32_t a=pointer32(&cpu,p);if(!a)stop("outside common 32-bit pointer domain");return a;}
static uint32_t get32(const void*p){uint32_t a;memcpy(&a,p,4);return a;}
static void set32(void*p,uint32_t a){memcpy(p,&a,4);}
static uint32_t call(void(*f)(CPU*),const uint32_t*a,int n,int stdcall,uint32_t this){
 uint32_t sp=cpu.esp;cpu.ecx=this;cpu.ebx=0x12345678;cpu.esi=0x23456789;cpu.edi=0x3456789a;cpu.ebp=0x456789ab;
 for(int i=n-1;i>=0;i--){cpu.esp-=4;memcpy(p32(cpu.esp),a+i,4);}f(&cpu);if(!stdcall)cpu.esp+=(uint32_t)n*4;original_calls++;
 if(cpu.esp!=sp||cpu.ebx!=0x12345678||cpu.esi!=0x23456789||cpu.edi!=0x3456789a||cpu.ebp!=0x456789ab)stop("guest caller ABI/register preservation");return cpu.eax;
}
static Obj*lookup(uint32_t p){for(int i=0;i<128;i++)if(objects[i].refs&&gp(&objects[i])==p)return objects+i;return NULL;}
static Obj*allocobj(unsigned kind){for(int i=0;i<128;i++)if(!objects[i].refs){Obj*o=objects+i;memset(o,0,sizeof(*o));o->refs=1;o->kind=kind;o->vt=gp(kind==1?vtdd:kind==2?vts:vtclip);return o;}return NULL;}
static Surface*surface(uint32_t p){Obj*o=lookup(p);return o&&o->kind==2?o->surface:NULL;}
uint32_t dd_unsupported_c(void){stop("unimplemented COM vtable slot");return NOIMPL;}
uint32_t dd_QueryInterface_c(uint32_t p,uint32_t iid,uint32_t out){
 Obj*o=lookup(p);if(!o||!iid||!out)return INVALID;set32(p32(out),0);
 static const unsigned char unknown[16]={0,0,0,0,0,0,0,0,0xc0,0,0,0,0,0,0,0x46};
 unsigned char own[16]={0x80,0xdb,0x14,0x6c,0x33,0xa7,0xce,0x11,0xa5,0x21,0,0x20,0xaf,0xb,0xe5,0x60};
 own[0]=o->kind==1?0x80:o->kind==2?0x81:0x85;
 if(memcmp(p32(iid),unknown,16)&&memcmp(p32(iid),own,16))return UINT32_C(0x80004002);
 o->refs++;set32(p32(out),p);com_calls[0]++;return 0;
}
uint32_t dd_AddRef_c(uint32_t p){Obj*o=lookup(p);if(!o)return 0;com_calls[1]++;return++o->refs;}
uint32_t dd_Release_c(uint32_t p){Obj*o=lookup(p);if(!o)return 0;com_calls[2]++;
 if(o->refs==1&&o->surface&&o->surface->locked)return 1;
 uint32_t n=--o->refs;if(!n){if(o->clipper)dd_Release_c(o->clipper);if(o->surface&&s23_free(o->surface))stop("release native surface");memset(o,0,sizeof(*o));}return n;}
static void desc(Obj*o,void*d){Surface*s=o->surface;uint32_t*a=d;memset(a,0,108);a[0]=108;a[1]=0x180f;a[2]=s->h;a[3]=s->w;a[4]=s->pitch;a[9]=s->guest;a[18]=32;a[19]=64;a[21]=s->fmt==32?32:16;uint32_t masks[4];if(s23_format(s,masks))stop("pixel format");for(int i=0;i<4;i++)a[22+i]=masks[i];a[26]=o->caps;}
uint32_t dd_CreateSurface_c(uint32_t p,uint32_t d,uint32_t out,uint32_t outer){
 Obj*o=lookup(p);if(!o||o->kind!=1||!d||!out||outer)return INVALID;set32(p32(out),0);const uint32_t*a=p32(d);
 if(a[0]!=108||(a[1]&0x1007)!=0x1007||a[18]!=32||a[19]!=64)return NOIMPL;
 int fmt=a[21]==32?32:a[21]==16&&a[22]==0x7c00&&a[23]==0x3e0&&a[24]==31?555:a[21]==16&&a[22]==0xf800&&a[23]==0x7e0&&a[24]==31?565:0;
 if(!fmt||(fmt==32&&(a[22]!=0xff0000||a[23]!=0xff00||a[24]!=255)))return NOIMPL;
 if(a[26]&~(0x40u|0x800u|0x200u))return NOIMPL;
 Surface*s=s23_create(a[3],a[2],a[3]*(fmt==32?4:2),fmt);if(!s)return FAIL;Obj*n=allocobj(2);if(!n){s23_free(s);return FAIL;}n->surface=s;n->caps=a[26];set32(p32(out),gp(n));com_calls[3]++;return 0;
}
uint32_t dd_CreateClipper_c(uint32_t p,uint32_t flags,uint32_t out,uint32_t outer){Obj*o=lookup(p);if(!o||o->kind!=1||flags||outer||!out)return INVALID;Obj*n=allocobj(3);if(!n)return FAIL;set32(p32(out),gp(n));com_calls[4]++;return 0;}
uint32_t dd_SetCooperativeLevel_c(uint32_t p,uint32_t hwnd,uint32_t flags){Obj*o=lookup(p);if(!o||o->kind!=1||!bound_window||hwnd!=1)return INVALID;if(flags!=8)return NOIMPL;o->hwnd=hwnd;com_calls[5]++;return 0;}
uint32_t dd_SetDisplayMode_c(uint32_t p,uint32_t w,uint32_t h,uint32_t bpp){(void)p;(void)w;(void)h;(void)bpp;return NOIMPL;}
uint32_t dd_Lock_c(uint32_t p,uint32_t rect,uint32_t d,uint32_t flags,uint32_t event){Obj*o=lookup(p);if(!o||o->kind!=2||!d||get32(p32(d))!=108)return INVALID;if(rect||event||(flags&~1u))return NOIMPL;uint32_t g;int pitch;if(s23_lock(o->surface,&g,&pitch))return FAIL;desc(o,p32(d));com_calls[6]++;return 0;}
uint32_t dd_Unlock_c(uint32_t p,uint32_t address){Surface*s=surface(p);if(!s||address!=s->guest)return INVALID;if(s23_unlock(s))return FAIL;com_calls[7]++;return 0;}
uint32_t dd_GetPixelFormat_c(uint32_t p,uint32_t out){Obj*o=lookup(p);if(!o||o->kind!=2||!out||get32(p32(out))!=32)return INVALID;uint32_t d[27];desc(o,d);memcpy(p32(out),d+18,32);com_calls[8]++;return 0;}
uint32_t dd_GetSurfaceDesc_c(uint32_t p,uint32_t out){Obj*o=lookup(p);if(!o||o->kind!=2||!out||get32(p32(out))!=108)return INVALID;desc(o,p32(out));com_calls[9]++;return 0;}
uint32_t dd_SetClipper_c(uint32_t p,uint32_t c){Obj*o=lookup(p),*cl=lookup(c);if(!o||o->kind!=2||(c&&(!cl||cl->kind!=3)))return INVALID;if(c)dd_AddRef_c(c);if(o->clipper)dd_Release_c(o->clipper);o->clipper=c;com_calls[10]++;return 0;}
uint32_t dd_SetHWnd_c(uint32_t p,uint32_t flags,uint32_t hwnd){Obj*o=lookup(p);if(!o||o->kind!=3||flags||hwnd!=1||!bound_window)return INVALID;o->hwnd=hwnd;com_calls[11]++;return 0;}
uint32_t dd_GetHWnd_c(uint32_t p,uint32_t out){Obj*o=lookup(p);if(!o||o->kind!=3||!out)return INVALID;set32(p32(out),o->hwnd);return 0;}
static int rect(uint32_t ptr,Surface*s,int*r){if(ptr)memcpy(r,p32(ptr),16);else{r[0]=r[1]=0;r[2]=s->w;r[3]=s->h;}return r[2]>=r[0]&&r[3]>=r[1];}
uint32_t dd_Blt_c(uint32_t p,uint32_t dr,uint32_t src,uint32_t sr,uint32_t flags,uint32_t fx){
 Surface*d=surface(p),*s=surface(src);if(!d||!s)return INVALID;if(flags||fx)return NOIMPL;int a[4],b[4];if(!rect(dr,d,a)||!rect(sr,s,b))return INVALID;
 if(a[2]-a[0]!=b[2]-b[0]||a[3]-a[1]!=b[3]-b[1])return NOIMPL;
 Obj*o=lookup(p);if(o->clipper){Obj*c=lookup(o->clipper);if(!c||c->hwnd!=1||!bound_window)return FAIL;}
 S23Rect r={b[0],b[1],b[2]-b[0],b[3]-b[1]};if(s23_blt(d,a[0],a[1],s,r,0,0))return FAIL;com_calls[12]++;return 0;
}
uint32_t dd_BltFast_c(uint32_t p,uint32_t px,uint32_t py,uint32_t src,uint32_t sr,uint32_t flags){Surface*d=surface(p),*s=surface(src);if(!d||!s)return INVALID;if(flags)return NOIMPL;int a[4];if(!rect(sr,s,a))return INVALID;S23Rect r={a[0],a[1],a[2]-a[0],a[3]-a[1]};if(s23_bltfast(d,px,py,s,r,0,0))return FAIL;com_calls[13]++;return 0;}
uint32_t dd_clock_c(void){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))stop("monotonic clock");return(uint32_t)((uint64_t)t.tv_sec*1000+t.tv_nsec/1000000);}
uint32_t dd_sprintf_c(uint32_t out,uint32_t fmt,uint32_t a,uint32_t b){return(uint32_t)snprintf(p32(out),88,p32(fmt),a,b);}
uint32_t dd_log_c(uint32_t text,uint32_t a,uint32_t b){if(a||b)stop("unsupported original graphics diagnostic format");return(uint32_t)printf("original DD: %s\n",(char*)p32(text));}
static void init(void){
 if(!s23_initialize()||!initialize_cpu(&cpu,manager)||cpu.pointer_offset!=s23_native_offset())stop("shared pointer domain init");if(ptr_initialize_pointers)ptr_initialize_pointers(cpu.pointer_offset);
 for(int i=0;i<23;i++)vtdd[i]=gp(dd_unsupported);for(int i=0;i<36;i++)vts[i]=gp(dd_unsupported);for(int i=0;i<9;i++)vtclip[i]=gp(dd_unsupported);
 uint32_t common[]={gp(dd_QueryInterface),gp(dd_AddRef),gp(dd_Release)};for(int i=0;i<3;i++)vtdd[i]=vts[i]=vtclip[i]=common[i];
 vtdd[4]=gp(dd_CreateClipper);vtdd[6]=gp(dd_CreateSurface);vtdd[20]=gp(dd_SetCooperativeLevel);vtdd[21]=gp(dd_SetDisplayMode);
 vts[5]=gp(dd_Blt);vts[7]=gp(dd_BltFast);vts[21]=gp(dd_GetPixelFormat);vts[22]=gp(dd_GetSurfaceDesc);vts[25]=gp(dd_Lock);vts[28]=gp(dd_SetClipper);vts[32]=gp(dd_Unlock);vtclip[5]=gp(dd_GetHWnd);vtclip[8]=gp(dd_SetHWnd);
 Obj*d=allocobj(1);if(!d)stop("DD allocation");set32(g_10de478,gp(d));set32(g_7a9534,gp(dd_clock));
 uint32_t args[]={gp(d),1,8};if(call(c_dd_SetCooperativeLevel,args,3,1,0))stop("DD cooperative/window binding");
}
static void load(const char*metadata,const char*file){
 FILE*f=fopen(metadata,"rb");if(!f)stop("SG3 open");unsigned char header[64];if(fread(header,64,1,f)!=1||get32(header+4)!=213||get32(header+16)!=372)stop("fixed original SG3 version");
 if(fseek(f,40680+201*64,SEEK_SET))stop("SG3 seek");for(int i=0;i<172;i++){if(fread(records+i*72,64,1,f)!=1)stop("SG3 record read");set32(records+i*72+12,16);memset(records+i*72+64,0,8);}fclose(f);
 f=fopen(file,"rb");if(!f)stop("555 open");size_t n=fread(pixels,1,sizeof(pixels),f);if(n!=2067457||fgetc(f)!=EOF)stop("fixed original 555 size");fclose(f);
 for(int i=0;i<172;i++){unsigned char*r=records+i*72;uint32_t off=get32(r),size=get32(r+4);if(off<39218||off+size>n)stop("SG3 sprite bounds");set32(r,gp(pixels+off));}
 unsigned char*g=manager+4096+16*52292;set32(g,gp(records));set32(g+52276,172);set32(g+52280,172);printf("native SG3 adapter: 172 original records, first sprite file offset 39218\n");
}
static uint32_t create(int w,int h,int bits){uint32_t a[]={w,h,0,0,bits,0x555};uint32_t p=call(c_create_original_surface,a,6,0,0);if(!p||!surface(p))stop("original DD CreateSurface helper");return p;}
static void setup(Surface*s,unsigned clipw,unsigned cliph){set32(g_10de000,s->pitch);set32(g_10de008,s->w);set32(g_1156df8,s->guest);set32(g_10c7324,0);set32(g_10c7320,clipw);set32(g_10c731c,0);set32(g_10c7318,cliph);set32(g_10de47c,0x555);set32(g_a9828c,0);g_10de045[0]=0;}
static void draw(int idx,int px,int py){uint32_t a[]={16*16384u+(uint32_t)idx,px,py,0,0,0};call(c_draw_base,a,6,1,gp(manager));call(c_draw_top,a,3,1,gp(manager));}
static void refresh(uint32_t src,uint32_t conv,uint32_t primary,int w,int h){
 set32(g_10de4a4,src);set32(g_10de4a8,conv);set32(g_10de4ac,primary);set32(g_10de0c8,32);set32(g_10de0cc,0);set32(g_10de018,h);set32(g_10de01c,w);uint32_t r[]={0,0,w,h};memcpy(g_10de460,r,16);uint32_t force[]={1};call(c_original_refresh,force,1,0,0);
 if(surface(src)->locked||surface(conv)->locked)stop("original refresh leaked lock");
}
#define CHECK(a) do{if(!(a))stop(#a);}while(0)
int stage24_verify(const char*fixture,const char*metadata,const char*file){
 init();load(metadata,file);FILE*f=fopen(fixture,"rb");CHECK(f&&fread(&count,4,1,f)==1&&count==11);unsigned checks=0;
 for(unsigned i=0;i<count;i++){uint32_t hd[6];CHECK(fread(hd,sizeof(hd),1,f)==1);int draws[20][3];CHECK(hd[4]+hd[5]<=20&&fread(draws,12,hd[4]+hd[5],f)==hd[4]+hd[5]);
  uint32_t src=create(hd[0],hd[1],16),conv=create(hd[0],hd[1],32),primary=create(hd[0],hd[1],32);Surface*s=surface(src),*c=surface(conv),*d=surface(primary);frames[i]=d;
  uint32_t args[6]={get32(g_10de478),0,gp(&output_object),0};CHECK(call(c_dd_CreateClipper,args,4,1,0)==0);uint32_t clip=output_object;args[0]=clip;args[1]=0;args[2]=1;CHECK(call(c_dd_SetHWnd,args,3,1,0)==0);args[0]=primary;args[1]=clip;CHECK(call(c_dd_SetClipper,args,2,1,0)==0);args[0]=clip;CHECK(call(c_dd_Release,args,1,1,0)==1);
  memset(s->pixels,0,s->pitch*s->h);for(int k=0;k<s->w*s->h;k++){uint16_t color=0x2947;memcpy(s->pixels+k*2,&color,2);}memset(c->pixels,0xa5,c->pitch*c->h);memset(d->pixels,0xa5,d->pitch*d->h);
  setup(s,hd[2],hd[3]);unsigned char*expected=malloc((size_t)hd[0]*hd[1]*4);CHECK(expected);initials[i]=malloc((size_t)d->pitch*d->h);CHECK(initials[i]);
  for(int phase=0;phase<2;phase++){
   unsigned start=phase?hd[4]:0,n=phase?hd[5]:hd[4];
   descriptor[0]=108;args[0]=src;args[1]=0;args[2]=gp(descriptor);args[3]=1;args[4]=0;CHECK(call(c_dd_Lock,args,5,1,0)==0);CHECK(descriptor[9]==s->guest&&descriptor[4]==(unsigned)s->pitch);
   CHECK(call(c_dd_Lock,args,5,1,0)==FAIL);checks++;
   for(unsigned j=start;j<start+n;j++)draw(draws[j][0],draws[j][1],draws[j][2]);args[0]=src;args[1]=s->guest;CHECK(call(c_dd_Unlock,args,2,1,0)==0);
   size_t bytes=(size_t)s->pitch*s->h;CHECK(fread(expected,bytes,1,f)==1);CHECK(!memcmp(s->pixels,expected,bytes));checks++;
   /* Isolated graphics readiness input, after real native window + surfaces exist.
      The complete WinMain initializer is not called or declared passed. */
   if(i==0&&phase==0){unsigned long locks=com_calls[6],blts=com_calls[12];CHECK(get32(g_854be0)==1);refresh(src,conv,primary,hd[0],hd[1]);CHECK(com_calls[6]==locks&&com_calls[12]==blts);for(int k=0;k<d->pitch*d->h;k++)CHECK(d->pixels[k]==0xa5);checks++;printf("PASS original refresh disabled state: zero COM drawing calls, all output retained\n");}
   CHECK(bound_window&&surface(src)&&surface(conv)&&surface(primary));set32(g_854be0,0);
   refresh(src,conv,primary,hd[0],hd[1]);bytes=(size_t)d->pitch*d->h;CHECK(fread(expected,bytes,1,f)==1);
   if(memcmp(d->pixels,expected,bytes)){size_t k=0,total=0;while(k<bytes&&d->pixels[k]==expected[k])k++;for(size_t j=0;j<bytes;j++)total+=d->pixels[j]!=expected[j];fprintf(stderr,"refresh mismatch case=%u phase=%d first=%zu got=%02x expected=%02x bytes=%zu flags=%u format=%u lock=%lu unlock=%lu blt=%lu dims=%u/%u convpixel=%08x primarypixel=%08x\n",i,phase,k,d->pixels[k],expected[k],total,get32(g_854be0),get32(g_10de0c8),com_calls[6],com_calls[7],com_calls[12],get32(g_10de018),get32(g_10de01c),get32(c->pixels),get32(d->pixels));FILE*dump=fopen("runs/stage24-sprites/failed-frame.bgra","wb");CHECK(dump);fwrite(d->pixels,bytes,1,dump);fclose(dump);stop("original refresh comparison");}checks++;
   if(!phase)memcpy(initials[i],d->pixels,bytes);
  }
  args[0]=src;CHECK(call(c_dd_Release,args,1,1,0)==0);args[0]=conv;CHECK(call(c_dd_Release,args,1,1,0)==0);free(expected);printf("PASS native complete sprite case %u: 16-bit surface + original refresh + COM Blt exact\n",i);
 }
 CHECK(fgetc(f)==EOF);fclose(f);printf("PASS stage24 CPU: %u cases, %u check groups, %lu native original/ABI calls; COM lock=%lu unlock=%lu blt=%lu\n",count,checks,original_calls,com_calls[6],com_calls[7],com_calls[12]);
 FILE*j=fopen("evidence/stage24-native.json","wb");CHECK(j);fprintf(j,"{\"cases\":%u,\"check_groups\":%u,\"original_and_ABI_calls\":%lu,\"COM_CreateSurface_calls\":%lu,\"COM_Lock_calls\":%lu,\"COM_Unlock_calls\":%lu,\"COM_Blt_calls\":%lu,\"pixel_byte_mismatches\":0,\"window_bound\":true,\"native_sprite_records\":172}\n",count,checks,original_calls,com_calls[3],com_calls[6],com_calls[7],com_calls[12]);fclose(j);return 0;
}
