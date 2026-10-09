/* Compile-time layout check against the fixed SR public helper header. */
#include <stddef.h>
#include "llasm_cpu.h"
_Static_assert(offsetof(_cpu,_eax)==0,"SR EAX ABI");
_Static_assert(offsetof(_cpu,_ecx)==4,"SR ECX ABI");
_Static_assert(offsetof(_cpu,_esp)==16,"SR ESP ABI");
_Static_assert(offsetof(_cpu,_eflags)==32,"SR EFLAGS ABI");
_Static_assert(offsetof(_cpu,_st)==48,"SR x87 surrogate array ABI");
_Static_assert(offsetof(_cpu,_pointer_offset)==112,"SR pointer-offset ABI");
/* LLVM uses120-byte prefix; the C helper struct has extra trailing fields. */
_Static_assert(offsetof(_cpu,_st_result)==120,"SR helper prefix ABI");
