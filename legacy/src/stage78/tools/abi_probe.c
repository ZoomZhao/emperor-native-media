/* Original approved header is used as a type specification only. */
#define PTROFS_64BIT
#include "llasm_cpu.h"
#include "guarded_helpers.h"
#include <stddef.h>
#include <stdio.h>
_Static_assert(sizeof(_cpu)==sizeof(S78CPU),"CPU container size");
_Static_assert(offsetof(_cpu,_eax)==offsetof(S78CPU,r),"eax ABI");
_Static_assert(offsetof(_cpu,_ecx)==offsetof(S78CPU,r)+4,"ecx ABI");
_Static_assert(offsetof(_cpu,_edi)==offsetof(S78CPU,r)+28,"edi ABI");
_Static_assert(offsetof(_cpu,_eflags)==offsetof(S78CPU,r)+32,"flags ABI");
_Static_assert(offsetof(_cpu,_st)==offsetof(S78CPU,st),"FP container ABI");
_Static_assert(offsetof(_cpu,_pointer_offset)==offsetof(S78CPU,pointer_offset),"pointer ABI");
int main(void){printf("ABI container verified: size=%zu flags=%zu st=%zu pointer_offset=%zu; no FP80 conversion/guest execution\n",sizeof(S78CPU),offsetof(S78CPU,r)+32,offsetof(S78CPU,st),offsetof(S78CPU,pointer_offset));return 0;}
