#ifndef GUEST_MEMORY_TEST_HOOKS_H
#define GUEST_MEMORY_TEST_HOOKS_H
/* Preinclude only in the authored provider. Wrappers call real libc/kernel
 * functions and can fail one selected call. No checkpoint or guest code. */
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
void *guest_test_malloc(size_t);
void guest_test_free(void *);
void *guest_test_mmap(void *, size_t, int, int, int, off_t);
int guest_test_munmap(void *, size_t);
int guest_test_mprotect(void *, size_t, int);
FILE *guest_test_fopen(const char *, const char *);
int guest_test_fclose(FILE *);
long guest_test_sysconf(int);
#define malloc guest_test_malloc
#define free guest_test_free
#define mmap guest_test_mmap
#define munmap guest_test_munmap
#define mprotect guest_test_mprotect
#define fopen guest_test_fopen
#define fclose guest_test_fclose
#define sysconf guest_test_sysconf
#endif
