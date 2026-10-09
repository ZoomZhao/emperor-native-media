#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
extern void*s84_import(const void*,size_t);
extern void s84_delete(void*);
extern int s84_sync_unlock(void*,uint32_t,const void*,size_t),s84_export(void*,void*,size_t);
int main(void){
 uint32_t image[780+4]={0};image[0]=0x44443239;image[1]=1;image[2]=image[3]=image[6]=image[7]=2;image[4]=image[8]=32;
 uint32_t*o=image+24;o[0]=2;o[1]=1;o[6]=o[7]=2;o[8]=8;o[9]=32;o[10]=1;
 void*s=s84_import(image,sizeof(image));assert(s);unsigned char pixels[16];memset(pixels,0x6a,16);
 assert(!s84_sync_unlock(s,65,pixels,16));assert(!s84_sync_unlock(s,2,pixels,15));assert(s84_sync_unlock(s,2,pixels,16));assert(!s84_sync_unlock(s,2,pixels,16));
 uint32_t out[784];assert(s84_export(s,out,sizeof(out)));assert(out[24+10]==0&&!memcmp((unsigned char*)out+3120,pixels,16));s84_delete(s);return 0;
}
