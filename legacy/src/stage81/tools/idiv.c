/* Signed EDX:EAX / r32. #DE stops before any architectural result commit.
 * Exception dispatch remains unimplemented; undefined flags are not promoted
 * into a universal flag contract. No host signed-overflow division is invoked. */
#include "runtime.h"
#include <limits.h>
#include <string.h>
extern void stage77_platform_boundary(unsigned,unsigned,unsigned) __attribute__((noreturn));
void x86_idiv_64(S78CPU *cpu,uint32_t bits){
 uint64_t raw=((uint64_t)cpu->r[S78_EDX]<<32)|cpu->r[S78_EAX];int64_t dividend;int32_t divisor;memcpy(&dividend,&raw,8);memcpy(&divisor,&bits,4);
 if(!divisor||(dividend==INT64_MIN&&divisor==-1))stage77_platform_boundary(100,0,bits);
 int64_t quotient=dividend/divisor,remainder=dividend%divisor;
 if(quotient<INT32_MIN||quotient>INT32_MAX)stage77_platform_boundary(100,0,bits);
 cpu->r[S78_EAX]=(uint32_t)(int32_t)quotient;cpu->r[S78_EDX]=(uint32_t)(int32_t)remainder;
}
