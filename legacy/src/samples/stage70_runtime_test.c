#include "stage70_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
    unsigned tests=0;uint8_t data[32]={0};S70Region r={0x1000,32,3,data};S70Memory m={&r,1,0,0};S70CPU c={0};
    for(unsigned bits=8;bits<=32;bits*=2)for(unsigned back=0;back<2;back++){
        memset(data,0,sizeof(data));memset(&c,0,sizeof(c));c.r[0]=0xa1b2c3d4;c.r[1]=3;c.r[7]=back?0x1017:0x1001;c.flags=0x203|(back?0x400:0);c.budget=10;
        uint32_t initial=c.r[7],flags=c.flags;assert(!s70_stos(&c,&m,bits,1));assert(c.r[1]==0&&c.r[7]==initial+(back?(uint32_t)-(3*(bits/8)):3*(bits/8))&&c.flags==flags);
        for(unsigned i=0;i<3;i++){uint32_t value=0;assert(!s70_read(&m,initial+(back?(uint32_t)-(i*(bits/8)):i*(bits/8)),bits,&value));assert(value==(0xa1b2c3d4&s70_mask(bits)));}tests++;
    }
    c.r[1]=0;c.r[7]=0xfffffff0;c.budget=0;assert(!s70_stos(&c,&m,32,1));tests++;
    c.r[0]=0x778899aa;c.r[1]=3;c.r[7]=0x101c;c.flags=0x202;c.budget=10;assert(s70_stos(&c,&m,32,1)==S70_FAULT&&c.r[1]==2&&c.r[7]==0x1020);tests++;
    c.r[1]=3;c.r[7]=0x1000;c.budget=1;assert(s70_stos(&c,&m,32,1)==S70_BUDGET&&c.r[1]==2&&c.r[7]==0x1004);c.budget=5;assert(!s70_stos(&c,&m,32,1)&&c.r[1]==0);tests++;
    c.r[1]=0x12345678;s70_setreg(&c,1,8,8,0xab);assert(c.r[1]==0x1234ab78);s70_setreg(&c,1,16,0,0xcdef);assert(c.r[1]==0x1234cdef);tests++;
    c.flags=0x400;s70_flags(&c,0x7f,1,0x80,8,0);assert((c.flags&0x881)==0x880&&(c.flags&0x400));tests++;
    c.flags=1;s70_flags(&c,0xff,1,0,8,0);assert((c.flags&0x41)==0x41);tests++;
    c.flags=1;s70_flags(&c,0x7fff,1,0x8000,16,3);assert((c.flags&0x881)==0x881);tests++;
    c.flags=0x11;c.defined_flags=~0u;s70_flags(&c,0x80,0xff,0x80,8,2);assert((c.flags&0x881)==0x80&&!(c.defined_flags&0x10));tests++;
    unsigned expected[16]={1,0,1,0,0,1,1,0,1,0,1,0,0,1,0,1};for(unsigned i=0;i<16;i++){assert(s70_condition(0x885,i)==(int)expected[i]);tests++;}
    printf("{\"passed\":true,\"cases\":%u,\"game_derived_object_linked\":false}\n",tests);return 0;
}
