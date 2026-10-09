/* Authored connection layer. Tests never link or load translated game code. */
#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static S80Runtime *active;
static int get(FILE *f,void *p,size_t n){return fread(p,1,n,f)==n;}
int s80_import_threads(S80Runtime *r,const char *path,uint64_t offset){
 memset(r,0,sizeof(*r));FILE *f=fopen(path,"rb");if(!f)return S80_CPU_FORMAT;char magic[4];uint32_t v,n,current;
 if(!get(f,magic,4)||memcmp(magic,"S80T",4)||!get(f,&v,4)||v!=1||!get(f,&n,4)||n!=4||!get(f,&current,4))goto fail;
 r->thread_set=(S79Threads){r->threads,4,4};
 for(unsigned i=0;i<4;i++){
  S79Thread *t=r->threads+i;S80Arch *a=r->arch+i;uint32_t status,teb;
  if(!get(f,&t->tid,4)||!get(f,&status,4)||status<1||status>3||!get(f,&t->last_error,4)||!get(f,&teb,4)||!get(f,&t->allocated,8)||!get(f,t->tls,256)||!get(f,t->core,40)||!get(f,a->selectors,24)||!get(f,&a->cr0,4)||!get(f,a->gdtr,32)||!get(f,a->ldtr,32)||!get(f,&a->fs_base,8)||a->fs_base!=teb||!get(f,&a->untrusted_gs,8)||!get(f,&a->fpcw,4)||!get(f,&a->fpsw,4)||!get(f,&a->fptag,4))goto fail;
  for(unsigned j=0;j<8;j++)if(!get(f,a->mantissa+j,8)||!get(f,a->exponent+j,2))goto fail;
  if(!get(f,t->opaque,5608))goto fail;
  a->status=status;t->public_CPU_known=t->cached_segments_proved=1;t->runnable=status==1;
  if(t->tid==current){if(r->thread_set.current!=4)goto fail;r->thread_set.current=i;}
  for(unsigned j=0;j<i;j++)if(r->threads[j].tid==t->tid)goto fail;
 }
 if(fgetc(f)!=EOF||r->thread_set.current==4||!r->threads[r->thread_set.current].runnable)goto fail;
 fclose(f);S79Thread *t=r->threads+r->thread_set.current;memcpy(r->cpu.r,t->core,32);r->cpu.r[8]=t->core[9];r->cpu.pointer_offset=offset;r->pc=t->core[8];r->cpu.st_cw=r->arch[r->thread_set.current].fpcw;return 0;
 fail:fclose(f);memset(r,0,sizeof(*r));return S80_CPU_FORMAT;
}
/* Exact initial-state roundtrip. Caller must use before runtime mutations.
 * An updated core alongside immutable initial opaque is a different contract. */
int s80_export_initial_threads(const S80Runtime *r,const char *path){
 FILE *f=fopen(path,"wbx");if(!f)return S80_CPU_FORMAT;uint32_t header[3]={1,4,r->threads[r->thread_set.current].tid};int ok=fwrite("S80T",1,4,f)==4&&fwrite(header,4,3,f)==3;
 for(unsigned i=0;i<4&&ok;i++){
  const S79Thread *t=r->threads+i;const S80Arch *a=r->arch+i;uint32_t prefix[4]={t->tid,a->status,t->last_error,(uint32_t)a->fs_base};
  ok=fwrite(prefix,4,4,f)==4&&fwrite(&t->allocated,8,1,f)==1&&fwrite(t->tls,4,64,f)==64&&fwrite(t->core,4,10,f)==10&&fwrite(a->selectors,4,6,f)==6&&fwrite(&a->cr0,4,1,f)==1&&fwrite(a->gdtr,8,4,f)==4&&fwrite(a->ldtr,8,4,f)==4&&fwrite(&a->fs_base,8,1,f)==1&&fwrite(&a->untrusted_gs,8,1,f)==1&&fwrite(&a->fpcw,4,1,f)==1&&fwrite(&a->fpsw,4,1,f)==1&&fwrite(&a->fptag,4,1,f)==1;
  for(unsigned j=0;j<8&&ok;j++)ok=fwrite(a->mantissa+j,8,1,f)==1&&fwrite(a->exponent+j,2,1,f)==1;
  ok=ok&&fwrite(t->opaque,1,5608,f)==5608;
 }
 if(fclose(f))ok=0;return ok?0:S80_CPU_FORMAT;
}
void s80_save_core(S80Runtime *r){S79Thread *t=r->threads+r->thread_set.current;memcpy(t->core,r->cpu.r,32);t->core[8]=r->pc;t->core[9]=r->cpu.r[8];/* opaque is immutable initial sidecar, NOT an updated engine context */}
int s80_switch_thread(S80Runtime *r,uint32_t tid){
 unsigned old=r->thread_set.current;int q=s79_thread_switch(&r->thread_set,tid);if(q)return q;
 r->thread_set.current=old;s80_save_core(r);s79_thread_switch(&r->thread_set,tid);S79Thread *t=r->threads+r->thread_set.current;memcpy(r->cpu.r,t->core,32);r->cpu.r[8]=t->core[9];r->pc=t->core[8];r->cpu.st_cw=r->arch[r->thread_set.current].fpcw;
 if(r->memory){r->helpers.fs_base=(uint32_t)r->arch[r->thread_set.current].fs_base;r->helpers.active_teb=r->helpers.fs_base;}return 0;
}
int s80_read(void *p,uint32_t a,size_t n,uint8_t *out){S80Runtime *r=p;if(s79_guard(&r->memory->view,a,n,1))return 0;if(n)memcpy(out,(void *)(uintptr_t)(r->memory->offset+a),n);return 1;}
int s80_write(void *p,uint32_t a,size_t n,const uint8_t *in){S80Runtime *r=p;if(n>4||r->pending||r->writes>=r->write_cap||s79_guard(&r->memory->view,a,n,2))return 0;if(n){uint32_t v=0;memcpy(&v,in,n);memcpy((void *)(uintptr_t)(r->memory->offset+a),in,n);r->trace[r->writes++]=(S80Write){r->pc,a,(uint32_t)n,v};}return 1;}
int s80_connect(S80Runtime *r,S80Sparse *m,S80Write *trace,size_t cap,uint64_t steps){
 if(!m||!m->view.base||m->offset!=r->cpu.pointer_offset||!trace||!cap)return S80_MEMORY;r->memory=m;r->trace=trace;r->write_cap=cap;r->step_cap=steps;
 S80Arch *a=r->arch+r->thread_set.current;r->helpers=(S78Context){s80_read,s80_write,r,steps,(uint32_t)a->fs_base,3,(uint32_t)a->fs_base,1,1,1};/* limit=3 is scoped FS:[0] extent, not hidden hardware limit */return 0;
}
static _Noreturn void stop(uint32_t code,uint32_t address){if(!active||!active->armed)abort();active->status=code;active->fault=address;longjmp(active->escape,1);}
void s80_activate(S80Runtime *r){active=r;}
int s80_set_bindings(S80Runtime *r,const S80Binding *b,size_t n,const S80Binding *t,size_t k){
 const S80Binding *sets[]={b,t};size_t counts[]={n,k};for(unsigned s=0;s<2;s++)for(size_t i=0;i<counts[s];i++){uintptr_t p=(uintptr_t)sets[s][i].target;if(p<r->cpu.pointer_offset||p-r->cpu.pointer_offset>UINT32_MAX)return S80_BINDING;for(size_t j=0;j<i;j++)if(sets[s][j].pc==sets[s][i].pc&&sets[s][j].target!=sets[s][i].target)return S80_BINDING;}
 r->returns=b;r->return_count=n;r->targets=t;r->target_count=k;return 0;
}
void stage72_step(uint32_t pc){if(!active)abort();if(active->pending)stop(S80_TRACE_PROTOCOL,active->pending_address);if(active->steps>=active->step_cap)stop(S80_BUDGET,pc);active->pc=pc;active->steps++;}
void stage72_guard(void *p,uint32_t n,uint32_t permission){uint32_t a;if(!active)abort();if(s79_host_to_guest(&active->memory->view,p,&a)||s79_guard(&active->memory->view,a,n,permission))stop(S80_MEMORY,0);if(permission&2){if(!n||n>4||active->pending)stop(S80_TRACE_PROTOCOL,a);if(active->writes>=active->write_cap)stop(S80_BUDGET,a);active->pending=1;active->pending_address=a;active->pending_size=n;}}
void stage72_note_write(void *p,uint32_t n,uint64_t v){uint32_t a;if(!active)abort();if(s79_host_to_guest(&active->memory->view,p,&a)||!active->pending||a!=active->pending_address||n!=active->pending_size)stop(S80_TRACE_PROTOCOL,0);uint32_t actual=0;memcpy(&actual,p,n);uint32_t mask=n==4?UINT32_MAX:((1u<<(8*n))-1);if(actual!=((uint32_t)v&mask))stop(S80_TRACE_PROTOCOL,a);active->trace[active->writes++]=(S80Write){active->pc,a,n,actual};active->pending=0;}
void *stage72_lookup_return(uint32_t pc){if(!active)abort();for(size_t i=0;i<active->return_count;i++)if(active->returns[i].pc==pc)return active->returns[i].target;stop(S80_BINDING,pc);}
/* Dynamic calls deliberately need a conditional domain; table membership alone
 * cannot authorize a target. Domain implementation is added in separate source. */
void *s80_bound_target(uint32_t pc){for(size_t i=0;i<active->target_count;i++)if(active->targets[i].pc==pc)return active->targets[i].target;stop(S80_BINDING,pc);}
S80Runtime *s80_active_runtime(void){return active;}
void s80_domain_stop(void){stop(S80_DYNAMIC_DOMAIN,active?active->pc:0);}
S78Context *stage78_current_context(void){return active?&active->helpers:NULL;}
void stage78_abort_boundary(int s){stop((uint32_t)s,0);}
S79Threads *stage79_active_threads(void){return active?&active->thread_set:NULL;}
void stage79_thread_boundary(int s){stop((uint32_t)s,0);}
