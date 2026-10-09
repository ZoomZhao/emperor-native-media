#ifndef STAGE10_NATIVE_PLATFORM_H
#define STAGE10_NATIVE_PLATFORM_H
#include <stddef.h>
#include <stdint.h>
/* Finite, captured Wine profile. Guest pointer conversion is in the LLasm
 * wrapper; registered host regions prevent unchecked access after conversion.
 * Unknown contracts stop the process with exit 86 before guest continuation.
 * A production runtime must replace that policy with its own boundary handler.
 */
void stage10_bind_buffer(void *buffer, size_t length, int writable);
uint32_t stage10_GetACP_c(void);
uint32_t stage10_GetVersion_c(void);
uint32_t stage10_GetLastError_c(void);
void stage10_SetLastError_c(uint32_t value);
uint32_t stage10_GetCPInfo_c(uint32_t code_page, void *output);
uint32_t stage10_GetVersionExA_c(void *output);
#endif
