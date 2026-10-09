/* Own pure PE-header query, current guest process only. No process launching. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
static uint16_t get16(const unsigned char *p) {return (uint16_t)p[0]|(uint16_t)p[1]<<8;}
static uint32_t get32(const unsigned char *p) {return (uint32_t)get16(p)|(uint32_t)get16(p+2)<<16;}
int s14_process_version(const unsigned char *image,size_t n,uint32_t pid,uint32_t current,uint32_t *value,uint32_t *error) {
    if(pid&&pid!=current) {
        if(pid==UINT32_MAX) {*value=0;*error=87;return 1;}
        return 0;
    }
    if(!image||n<64||memcmp(image,"MZ",2))return 0;
    uint32_t pe=get32(image+0x3c);
    if(pe>n||n-pe<24+52||memcmp(image+pe,"PE\0\0",4))return 0;
    const unsigned char *opt=image+pe+24;
    if(get16(image+pe+4)!=0x14c||get16(image+pe+20)<52||get16(opt)!=0x10b)return 0;
    *value=(uint32_t)get16(opt+48)<<16|get16(opt+50);return 1;
}
