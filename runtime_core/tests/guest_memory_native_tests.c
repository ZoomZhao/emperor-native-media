#include "guest_memory.h"
#include <errno.h>
#include <inttypes.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define GUEST_SPACE (UINT64_C(1) << 32)
enum Fault { NO_FAULT, ALLOCATION, RESERVATION, PROTECTION, FILE_OPEN, INVALID_NATIVE_PAGE_SIZE };
static struct {
    enum Fault fault;
    unsigned fail_call, fault_hits, allocation_calls, map_calls, protection_calls, open_calls;
    unsigned allocations, reservations, files;
    void *allocation, *reservation;
    FILE *file;
} ledger;
static unsigned checks;
static FILE *trace;

static void require(int value, const char *message) {
    ++checks;
    if (!value) { fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}
static void balanced(void) {
    require(!ledger.allocations && !ledger.reservations && !ledger.files,
            "real region allocation, reservation and file resources released");
}
static void reset_fault(enum Fault fault, unsigned call) {
    balanced(); memset(&ledger, 0, sizeof(ledger));
    ledger.fault = fault; ledger.fail_call = call;
    fprintf(trace, "{\"fault\":%u,\"fail_call\":%u}\n", (unsigned)fault, call);
}
void *guest_test_malloc(size_t n) {
    ++ledger.allocation_calls;
    if (ledger.fault == ALLOCATION && ledger.allocation_calls == ledger.fail_call) {
        ++ledger.fault_hits; errno = ENOMEM; return NULL;
    }
    void *p = malloc(n);
    if (p) {
        require(!ledger.allocations, "one live provider region allocation");
        ledger.allocation = p; ++ledger.allocations;
    }
    return p;
}
void guest_test_free(void *p) {
    if (p) {
        require(ledger.allocations == 1 && p == ledger.allocation, "free owned region allocation once");
        --ledger.allocations; ledger.allocation = NULL;
    }
    free(p);
}
void *guest_test_mmap(void *address, size_t n, int protection, int flags, int fd, off_t offset) {
    ++ledger.map_calls;
    require(!address && n == GUEST_SPACE && protection == PROT_NONE && fd == -1 && offset == 0,
            "reserve anonymous guest space without executable permission");
    require((flags & (MAP_PRIVATE | MAP_ANON)) == (MAP_PRIVATE | MAP_ANON), "private anonymous reservation");
    if (ledger.fault == RESERVATION && ledger.map_calls == ledger.fail_call) {
        ++ledger.fault_hits; errno = ENOMEM; return MAP_FAILED;
    }
    void *p = mmap(address, n, protection, flags, fd, offset);
    if (p != MAP_FAILED) {
        require(!ledger.reservations, "one live anonymous reservation");
        ledger.reservation = p; ++ledger.reservations;
    }
    return p;
}
int guest_test_munmap(void *p, size_t n) {
    require(ledger.reservations == 1 && p == ledger.reservation && n == GUEST_SPACE,
            "release exactly the owned anonymous reservation");
    int result = munmap(p, n);
    require(result == 0, "real kernel munmap succeeds");
    --ledger.reservations; ledger.reservation = NULL;
    return result;
}
int guest_test_mprotect(void *p, size_t n, int protection) {
    ++ledger.protection_calls;
    require(!(protection & PROT_EXEC), "provider never requests executable backing");
    if (ledger.fault == PROTECTION && ledger.protection_calls == ledger.fail_call) {
        ++ledger.fault_hits; errno = EACCES; return -1;
    }
    return mprotect(p, n, protection);
}
FILE *guest_test_fopen(const char *path, const char *mode) {
    ++ledger.open_calls;
    require(!strcmp(path, "fixtures/full.bin") || !strcmp(path, "fixtures/short.bin"),
            "provider reads only declared self-authored fixtures");
    require(!strcmp(mode, "rb"), "fixture is read-only input");
    if (ledger.fault == FILE_OPEN && ledger.open_calls == ledger.fail_call) {
        ++ledger.fault_hits; errno = EMFILE; return NULL;
    }
    FILE *file = fopen(path, mode);
    if (file) {
        require(!ledger.files, "one live provider fixture file");
        ledger.file = file; ++ledger.files;
    }
    return file;
}
int guest_test_fclose(FILE *file) {
    require(ledger.files == 1 && file == ledger.file, "close owned input file once");
    int result = fclose(file);
    require(result == 0, "real fixture fclose succeeds");
    --ledger.files; ledger.file = NULL;
    return result;
}
long guest_test_sysconf(int name) {
    if (ledger.fault == INVALID_NATIVE_PAGE_SIZE && name == _SC_PAGESIZE) {
        ++ledger.fault_hits; return 4097;
    }
    return sysconf(name);
}
static int open_plan(S79Memory *m, const S79Region *regions, size_t count, const char *file) {
    fprintf(trace, "{\"open\":{\"count\":%zu,\"file\":\"%s\",\"regions\":[", count, file ? file : "none");
    if (count <= 64) for (size_t i = 0; i < count; ++i) {
        const S79Region *r = regions + i;
        fprintf(trace, "%s[%" PRIu32 ",%" PRIu64 ",%u,%" PRIu64 "]", i ? "," : "", r->low, r->end, r->permissions, r->offset);
    }
    fputs("]}}\n", trace);
    return s79_memory_open(m, regions, count, file);
}
static int guard(S79Memory *m, uint32_t address, uint64_t n, unsigned permission) {
    fprintf(trace, "{\"guard\":[%" PRIu32 ",%" PRIu64 ",%u]}\n", address, n, permission);
    return s79_guard(m, address, n, permission);
}
static int read_bytes(S79Memory *m, uint32_t address, size_t n, uint8_t *out) {
    fprintf(trace, "{\"read\":[%" PRIu32 ",%zu]}\n", address, n);
    return s79_read(m, address, n, out);
}
static int write_bytes(S79Memory *m, uint32_t address, size_t n, const uint8_t *in) {
    fprintf(trace, "{\"write\":[%" PRIu32 ",%zu],\"bytes\":\"", address, n);
    if (n <= 32) for (size_t i = 0; i < n; ++i) fprintf(trace, "%02x", in[i]);
    fputs("\"}\n", trace);
    return s79_write(m, address, n, in);
}
static void closed(const S79Memory *m) {
    require(!m->base && !m->regions && !m->count && !m->writes && !m->poisoned,
            "closed or failed memory object has no live fields");
    balanced();
}
static int native_protection(const S79Memory *m, uint32_t offset) {
    mach_vm_address_t query = (mach_vm_address_t)(uintptr_t)(m->base + offset);
    mach_vm_address_t address = query; mach_vm_size_t size = 0;
    vm_region_basic_info_data_64_t info;
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t object = MACH_PORT_NULL;
    kern_return_t result = mach_vm_region(mach_task_self(), &address, &size, VM_REGION_BASIC_INFO_64,
                                         (vm_region_info_t)&info, &count, &object);
    if (object != MACH_PORT_NULL) mach_port_deallocate(mach_task_self(), object);
    require(result == KERN_SUCCESS && address <= query && query - address < size,
            "inspect only this process's owned native mapping");
    return info.protection;
}
static const S79Region fragments[] = {
    {0x1000, 1, 0x2000, 0}, {0x2000, 3, 0x3000, 0}, {0x4000, 3, 0x5000, 0}
};
static void boundaries(long page) {
    reset_fault(NO_FAULT, 0); S79Memory m = {0};
    require(open_plan(&m, fragments, 3, NULL) == S79_OK, "open self-authored adjacent and separated fragments");
    uint8_t out[32]; memset(out, 0xa5, sizeof(out));
    require(read_bytes(&m, 0x1ff0, sizeof(out), out), "read across adjacent R and RW fragments");
    for (size_t i = 0; i < sizeof(out); ++i) require(out[i] == 0, "new anonymous fragments begin zeroed");
    const uint8_t pattern[4] = {0x11, 0x22, 0x33, 0x44};
    require(write_bytes(&m, 0x2000, sizeof(pattern), pattern) && m.writes == 1, "write only the RW fragment");
    require(!write_bytes(&m, 0x1fff, sizeof(pattern), pattern) && m.writes == 1, "cross-region write refuses partial mutation");
    require(guard(&m, 0x1fff, 2, 2) == S79_PERMISSION, "cross-region write requires both fragments' permission");
    memset(out, 0xa5, sizeof(out));
    require(!read_bytes(&m, 0x2ff0, sizeof(out), out), "cross-hole read rejected before any copy");
    for (size_t i = 0; i < sizeof(out); ++i) require(out[i] == 0xa5, "failed read preserves all output bytes");
    require(!write_bytes(&m, 0x2ffe, sizeof(pattern), pattern) && m.writes == 1, "cross-hole write leaves state intact");
    require(guard(&m, UINT32_MAX, 2, 1) == S79_OVERFLOW && guard(&m, 0x2000, UINT64_MAX, 1) == S79_OVERFLOW,
            "guest-end and 64-bit length overflow rejected");
    require(guard(&m, 0x2000, 1, 4) == S79_OVERFLOW && guard(&m, 0x2000, 1, 0) == S79_OVERFLOW,
            "execute and empty permission requests unsupported");
    require(read_bytes(&m, 0x3000, 0, NULL) && write_bytes(&m, 0x3000, 0, NULL) && m.writes == 1,
            "zero-length operations touch neither buffers nor counter");
    require(read_bytes(&m, 0x2000, 4, out) && !memcmp(pattern, out, 4), "failed operations preserve prior bytes");
    int first = native_protection(&m, 0x1000), writable = native_protection(&m, 0x2000);
    int shared_page = UINT64_C(0x1000)/(uint64_t)page == UINT64_C(0x2000)/(uint64_t)page;
    require(first == (shared_page ? (VM_PROT_READ | VM_PROT_WRITE) : VM_PROT_READ),
            "native page union keeps adjacent writable fragment usable");
    require(writable == (VM_PROT_READ | VM_PROT_WRITE), "actual kernel RW protection excludes X");
    require(native_protection(&m, 0x10000) == VM_PROT_NONE, "untouched native pages retain no access");
    require(guard(&m, 0x3000, 1, 1) == S79_UNMAPPED, "guest hole denied despite wider native page union");
    m.poisoned = 1;
    require(guard(&m, 0x2000, 1, 1) == S79_UNMAPPED && !read_bytes(&m, 0x2000, 4, out), "poisoned mapping denies access");
    s79_memory_close(&m); closed(&m); s79_memory_close(&m); closed(&m);

    S79Region top = {UINT32_MAX - 4095u, 3, GUEST_SPACE, 0};
    require(open_plan(&m, &top, 1, NULL) == S79_OK, "map final page exactly to the 32-bit exclusive end");
    require(write_bytes(&m, UINT32_MAX - 3u, 4, pattern) && read_bytes(&m, UINT32_MAX - 3u, 4, out) && !memcmp(out, pattern, 4),
            "valid last byte usable without guest arithmetic wrap");
    uint32_t address = 0;
    require(s79_host_to_guest(&m, m.base + UINT32_MAX, &address) == S79_OK && address == UINT32_MAX,
            "native pointer conversion retains all guest address bits");
    address = 77;
    require(s79_host_to_guest(&m, m.base + GUEST_SPACE, &address) == S79_OVERFLOW && address == 77,
            "one-past reservation rejects before truncation and preserves output");
    s79_memory_close(&m); closed(&m);
}
static void invalid_plans(void) {
    reset_fault(NO_FAULT, 0); S79Memory m = {0}; S79Region r = fragments[0];
    require(open_plan(&m, NULL, 0, NULL) == S79_INVALID_PLAN, "empty plan rejected"); closed(&m);
    require(open_plan(&m, &r, 65, NULL) == S79_INVALID_PLAN, "oversized plan rejected before array access"); closed(&m);
    const S79Region bad[] = {{0x1001,1,0x2000,0},{0x1000,1,0x1000,0},
                             {0x1000,0,0x2000,0},{0x1000,8,0x2000,0},
                             {0x1000,1,GUEST_SPACE+4096,0}};
    for (size_t i = 0; i < sizeof(bad)/sizeof(bad[0]); ++i) {
        require(open_plan(&m, bad+i, 1, NULL) == S79_INVALID_PLAN, "invalid alignment, extent or permission rejected"); closed(&m);
    }
    const S79Region overlap[] = {{0x1000,3,0x3000,0},{0x2000,3,0x4000,0}};
    require(open_plan(&m, overlap, 2, NULL) == S79_INVALID_PLAN, "overlapping regions rejected"); closed(&m);
    require(!ledger.allocation_calls && !ledger.map_calls, "invalid plans allocate no resources");
}
static void failure_cleanup(void) {
    const struct {enum Fault fault; unsigned call;} failures[] = {
        {ALLOCATION,1}, {RESERVATION,1}, {FILE_OPEN,1}, {INVALID_NATIVE_PAGE_SIZE,1}, {PROTECTION,1}, {PROTECTION,2}
    };
    S79Region r = {0x1000,1,0x2000,0}; S79Memory m = {0};
    for (size_t i = 0; i < sizeof(failures)/sizeof(failures[0]); ++i) {
        reset_fault(failures[i].fault, failures[i].call);
        require(open_plan(&m, &r, 1, "fixtures/full.bin") == S79_IO, "injected OS/libc failure reported");
        require(ledger.fault_hits == 1, "selected failure path actually reached once");
        closed(&m); s79_memory_close(&m); closed(&m);
    }
    reset_fault(NO_FAULT,0);
    require(open_plan(&m, &r, 1, "fixtures/short.bin") == S79_IO, "actual truncated self-authored file fails"); closed(&m);
    r.offset = UINT64_MAX;
    require(open_plan(&m, &r, 1, "fixtures/full.bin") == S79_IO, "invalid self-file offset releases resources"); closed(&m);
    r.offset = 0;
    require(open_plan(&m, &r, 1, "fixtures/full.bin") == S79_OK, "successful import recovers after failures");
    uint8_t out[4]; require(read_bytes(&m, 0x1000, 4, out) && out[0] == 0x20 && out[1] == 0x21 && out[2] == 0x22 && out[3] == 0x23,
                            "only self-authored fixture bytes imported");
    require(native_protection(&m, 0x1000) == VM_PROT_READ, "readonly import is actually kernel readonly and NX");
    s79_memory_close(&m); closed(&m);
}
static void repeated_lifecycle(void) {
    reset_fault(NO_FAULT,0); S79Memory m = {0}; const uint8_t pattern[] = {1,2,3,4}; uint8_t out[4];
    for (unsigned i = 0; i < 64; ++i) {
        require(open_plan(&m, fragments,3,NULL) == S79_OK && !m.writes, "reopen closed object with fresh state");
        require(write_bytes(&m,0x2000,4,pattern) && m.writes==1, "each lifecycle owns its write count");
        s79_memory_close(&m); closed(&m); s79_memory_close(&m); closed(&m);
        memset(out,0xa5,sizeof(out));
        require(!read_bytes(&m,0x2000,4,out) && out[0]==0xa5 && out[3]==0xa5, "closed object cannot expose old bytes");
    }
    require(ledger.map_calls == 64 && ledger.allocation_calls == 64, "all repeated native lifecycles exercised");
}
int main(void) {
    trace = fopen("inputs.jsonl","wb"); if (!trace) return 2;
    long page = sysconf(_SC_PAGESIZE);
    require(page >= 4096 && (page & (page-1)) == 0, "supported native page size");
    boundaries(page); invalid_plans(); failure_cleanup(); repeated_lifecycle(); balanced();
    require(fclose(trace) == 0, "close actual-input trace");
    printf("{\"passed\":true,\"assertions\":%u,\"native_page_bytes\":%ld,\"cycles\":64,"
           "\"cross_region\":true,\"native_permissions_and_nx\":true,\"overflow\":true,"
           "\"failure_cleanup\":true,\"repeated_lifecycle\":true,"
           "\"GameData\":false,\"checkpoint\":false,\"guest_code\":false,\"ui\":false,\"audio_devices\":false}\n", checks,page);
    return 0;
}
