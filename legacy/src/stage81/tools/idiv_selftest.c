#include "runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void x86_idiv_64(S78CPU *,uint32_t);
int main(int argc,char **argv){assert(argc==3);S80Runtime r;memset(&r,0,sizeof(r));s80_activate(&r);FILE *f=fopen(argv[1],"rb"),*out=fopen(argv[2],"wbx");assert(f&&out);char magic[4];uint32_t n;assert(fread(magic,1,4,f)==4&&!memcmp(magic,"S81D",4)&&fread(&n,4,1,f)==1);assert(fwrite("S81D",1,4,out)==4&&fwrite(&n,4,1,out)==1);
 for(uint32_t i=0;i<n;i++){uint32_t input[4];assert(fread(input,4,4,f)==4);r.cpu.r[S78_EAX]=input[0];r.cpu.r[S78_EDX]=input[1];r.cpu.r[S78_FLAGS]=input[3];r.status=0;r.armed=1;int fault=setjmp(r.escape);if(!fault)x86_idiv_64(&r.cpu,input[2]);r.armed=0;assert(!fault||r.status==300);uint32_t result[4]={r.cpu.r[S78_EAX],r.cpu.r[S78_EDX],r.cpu.r[S78_FLAGS],(uint32_t)(fault!=0)};assert(fwrite(result,4,4,out)==4);
 }
 assert(fgetc(f)==EOF&&!fclose(f)&&!fclose(out));puts("PASS authored native signed division cases; no game object or original instructions");return 0;}
