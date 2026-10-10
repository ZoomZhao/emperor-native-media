#include "memory.hpp"
#include "thread_state.hpp"
#include "../../scripts/self_fixture_evidence.hpp"
#include <cstdio>
#include <stdexcept>

namespace {
size_t assertions = 0;
void require(bool value, const char* description) {
    ++assertions;
    if (!value) throw std::runtime_error(description);
}

int tracedImport(S77State* out,const S77Heap* heaps,size_t nh,const S77Allocation* rows,size_t nr,
                 uint32_t next,uint32_t mapped,S77Memory memory) {
    std::vector<uint64_t> values{nh,nr,next,mapped,out?out->next:0,out?out->count:0};
    if(heaps&&nh<=4096)for(size_t i=0;i<nh;++i){values.push_back(heaps[i].handle);values.push_back(heaps[i].options);}
    if(rows&&nr<=1000000)for(size_t i=0;i<nr;++i){values.push_back(rows[i].pointer);values.push_back(rows[i].heap);values.push_back(rows[i].size);values.push_back(rows[i].live);}
    self_fixture::numbers("heap-import-u64le",values);
    return ::s77_import(out,heaps,nh,rows,nr,next,mapped,memory);
}
int tracedAlloc(S77State* s,uint32_t heap,uint32_t flags,uint32_t size,uint32_t* result){
    self_fixture::numbers("heap-alloc-u64le",{heap,flags,size});return ::s77_alloc(s,heap,flags,size,result);
}
int tracedRealloc(S77State* s,uint32_t heap,uint32_t flags,uint32_t p,uint32_t size,uint32_t* result){
    self_fixture::numbers("heap-realloc-u64le",{heap,flags,p,size});return ::s77_realloc(s,heap,flags,p,size,result);
}
int tracedFree(S77State* s,uint32_t heap,uint32_t flags,uint32_t p){
    self_fixture::numbers("heap-free-u64le",{heap,flags,p});return ::s77_free(s,heap,flags,p);
}
int tracedSize(const S77State* s,uint32_t heap,uint32_t flags,uint32_t p,uint32_t* result){
    self_fixture::numbers("heap-size-u64le",{heap,flags,p});return ::s77_size(s,heap,flags,p,result);
}
int tracedDestroy(S77State* s,uint32_t heap){
    self_fixture::numbers("heap-retire-u64le",{heap});return ::s77_destroy_heap(s,heap);
}
size_t tracedExport(const S77State* s,S77Allocation* out,size_t room){
    self_fixture::numbers("heap-export-u64le",{room,s?s->count:0,s?s->poisoned:0,out?1u:0u});return ::s77_export_live(s,out,room);
}
void tracedDispose(S77State* s){
    self_fixture::numbers("heap-dispose-u64le",{s?s->count:0,s?s->heap_count:0});::s77_dispose(s);
}

void heapAndMemory() {
    constexpr uint32_t base = 0x10000000;
    runtime_core::Memory memory(base, 16u * 1024u * 1024u);
    const S77Heap heaps[] = {{1,0}, {2,0}};
    S77State state{};
    struct Cleanup { S77State& value; ~Cleanup() { tracedDispose(&value); } } cleanup{state};
    require(tracedImport(&state, heaps, 2, nullptr, 0, base, base, memory.heapAdapter()) == S77_OK,
            "empty self-authored heap state");
    uint32_t p = 0;
    require(tracedAlloc(&state, 1, 8, 3, &p) == S77_OK && p == base, "logical allocation and alignment");
    std::vector<uint8_t> bytes;
    require(memory.read(p, 8, bytes) && bytes == std::vector<uint8_t>(8,0), "zero-initialized allocation");
    require(memory.mappedBytes() == 16u * 1024u * 1024u, "finite mapping prefix");
    uint32_t size = 0;
    require(tracedSize(&state, 1, 0, p, &size) == S77_OK && size == 3, "logical requested size");
    require(tracedFree(&state, 2, 0, p) == S77_BOUNDARY, "wrong heap cannot free");
    require(memory.fill(p, 3, 0x6a), "self pattern write");
    uint32_t grown = 0;
    require(tracedRealloc(&state, 1, 8, p, 12, &grown) == S77_OK && grown != p, "reallocation uses own vector");
    require(memory.read(grown, 12, bytes) && bytes[0] == 0x6a && bytes[2] == 0x6a && bytes[3] == 0,
            "reallocation preserves prefix and zeroes tail");
    require(tracedSize(&state, 1, 0, p, &size) == S77_BOUNDARY, "old ownership retired");
    require(tracedFree(&state, 1, 0, grown) == S77_OK, "free own record");
    require(tracedFree(&state, 1, 0, grown) == S77_BOUNDARY, "double free rejected");
    require(tracedExport(&state, nullptr, 0) == 0, "no live allocation metadata");
    const auto writes = memory.writes();
    require(!memory.fill(base + 16u * 1024u * 1024u - 1, 2, 0xff), "full write extent checked");
    require(!memory.copy(base, base - 1, 2), "invalid copy source rejected before mutation");
    require(memory.writes() == writes, "failed writes do not mutate counter");
    require(!memory.check(base, UINT64_MAX, 1), "length overflow rejected");
    require(!memory.check(base, 1, 4), "execute permission unsupported");
    require(!memory.map(base, base + 8), "mapping cannot replace existing prefix");
    require(tracedDestroy(&state, 1) == S77_OK && state.heap_count == 1, "finite model heap retirement");
}
struct FaultMemory {
    runtime_core::Memory memory{0x10000000, 16u * 1024u * 1024u};
    bool failMap = false, failFill = false, failCopy = false;
    S77Memory adapter() {
        return {this,
            [](void* p, uint32_t a, uint32_t n, unsigned permissions) -> int {
                return static_cast<FaultMemory*>(p)->memory.check(a,n,permissions);
            },
            [](void* p, uint32_t begin, uint32_t end) -> int {
                auto& self=*static_cast<FaultMemory*>(p);
                return !self.failMap && self.memory.map(begin,end);
            },
            [](void* p, uint32_t a, uint32_t n, unsigned char value) -> int {
                auto& self=*static_cast<FaultMemory*>(p);
                return !self.failFill && self.memory.fill(a,n,value);
            },
            [](void* p, uint32_t dst, uint32_t src, uint32_t n) -> int {
                auto& self=*static_cast<FaultMemory*>(p);
                return !self.failCopy && self.memory.copy(dst,src,n);
            }};
    }
};
void importAndFailureBoundaries() {
    constexpr uint32_t base=0x10000000;
    runtime_core::Memory memory(base,16u*1024u*1024u);
    S77State state{};
    const S77Heap duplicate[]={{1,0},{1,0}};
    require(tracedImport(&state,duplicate,2,nullptr,0,base,base,memory.heapAdapter())==S77_METADATA,
            "duplicate heap handles rejected");
    require(!state.heaps&&!state.allocations&&!state.count,"failed import is transactional");
    const S77Heap heaps[]={{1,0},{2,0}};
    require(tracedImport(&state,heaps,2,nullptr,0,base,base,memory.heapAdapter())==S77_OK,"two synthetic heaps");
    struct Cleanup {S77State& value;~Cleanup(){tracedDispose(&value);}} cleanup{state};
    require(tracedImport(&state,heaps,2,nullptr,0,base,base,memory.heapAdapter())==S77_METADATA,"populated output rejected");
    uint32_t first=0,other=0;
    require(tracedAlloc(&state,1,8,8,&first)==S77_OK&&tracedAlloc(&state,2,8,8,&other)==S77_OK,"separate heap ownership");
    const auto next=state.next; const auto writes=memory.writes(); uint32_t result=91;
    require(tracedAlloc(&state,1,0,UINT32_MAX,&result)==S77_OK&&result==0,"overflow-sized allocation becomes finite OOM");
    require(state.next==next&&memory.writes()==writes,"OOM preserves metadata and bytes");
    result=91;
    require(tracedRealloc(&state,1,0,first,UINT32_MAX,&result)==S77_OK&&result==0,"realloc OOM");
    uint32_t size=0;
    require(tracedSize(&state,1,0,first,&size)==S77_OK&&size==8,"realloc OOM preserves prior ownership");
    require(tracedDestroy(&state,1)==S77_OK,"retire only selected heap");
    require(tracedSize(&state,1,0,first,&size)==S77_BOUNDARY,"retired heap allocation rejected");
    require(tracedSize(&state,2,0,other,&size)==S77_OK&&size==8,"other heap remains usable");
    require(tracedDestroy(&state,1)==S77_BOUNDARY,"repeated retirement rejected");
    S77Allocation guard{77,77,77,77};
    require(tracedExport(&state,&guard,0)==1&&guard.pointer==77,"limited export reports count without writing");
    require(tracedDestroy(&state,2)==S77_OK&&tracedExport(&state,nullptr,0)==0,"retire final heap");
    result=91;require(tracedAlloc(&state,2,0,1,&result)==S77_BOUNDARY&&result==91,"retired state leaves output unchanged");
    require(tracedExport(nullptr,&guard,1)==0&&guard.pointer==77,"null export safe");
    tracedDispose(&state);tracedDispose(&state);
    require(!state.heaps&&!state.allocations&&!state.next,"dispose idempotent and cleared");
    S77State notEmpty{};notEmpty.next=base;
    require(tracedImport(&notEmpty,heaps,2,nullptr,0,base,base,memory.heapAdapter())==S77_METADATA&&notEmpty.next==base,
            "nonzero empty output cannot be clobbered");
    require(memory.map(base+16u*1024u*1024u,base+16u*1024u*1024u),"zero length map at existing end");
    for(uint32_t i=0;i<8;++i)require(memory.fill(other+i,1,static_cast<uint8_t>(i+1)),"self byte pattern");
    require(memory.copy(other+1,other,7),"overlap uses memmove within private bytes");
    std::vector<uint8_t> bytes;
    require(memory.read(other,8,bytes)&&bytes==std::vector<uint8_t>({1,1,2,3,4,5,6,7}),"independent overlap oracle");
    for(int mode=0;mode<3;++mode) {
        self_fixture::numbers("callback-fault-mode-u64le",{static_cast<uint64_t>(mode)});
        FaultMemory fault; S77State poisoned{};
        require(tracedImport(&poisoned,heaps,2,nullptr,0,base,base,fault.adapter())==S77_OK,"fault fixture import");
        struct Dispose {S77State& value;~Dispose(){tracedDispose(&value);}} dispose{poisoned};
        uint32_t p=0;
        if(mode==2) {
            require(tracedAlloc(&poisoned,1,8,8,&p)==S77_OK,"copy fault setup");
            fault.failCopy=true;
            require(tracedRealloc(&poisoned,1,8,p,16,&result)==S77_MEMORY,"copy failure reported");
        } else {
            fault.failMap=mode==0;fault.failFill=mode==1;result=91;
            require(tracedAlloc(&poisoned,1,8,8,&result)==S77_MEMORY&&result==91,"map/fill failure reported without result");
        }
        require(poisoned.poisoned,"callback failure poisons ownership state");
        result=91;
        require(tracedAlloc(&poisoned,1,8,8,&result)==S77_BOUNDARY&&result==91,"poisoned state cannot continue");
        require(tracedExport(&poisoned,&guard,1)==0&&guard.pointer==77,"poisoned export cannot claim valid snapshot");
    }
}
void memoryBoundaries() {
    for(const auto& item:std::vector<std::pair<uint32_t,size_t>>{{0,0},{0,33u*1024u*1024u},{UINT32_MAX,1},{UINT32_MAX-7,8}}) {
        self_fixture::numbers("memory-extent-u64le",{item.first,item.second});
        bool rejected=false;
        try {runtime_core::Memory invalid(item.first,item.second);} catch(const std::invalid_argument&) {rejected=true;}
        require(rejected,"invalid or unrepresentable memory extent rejected");
    }
    runtime_core::Memory memory(64,16);
    std::vector<uint8_t> result{7,8};
    require(!memory.read(64,1,result)&&result==std::vector<uint8_t>({7,8}),"unmapped read preserves output");
    require(memory.map(64,80)&&memory.read(64,16,result)&&result==std::vector<uint8_t>(16,0),"fresh mapped vector cleared");
    result={7,8};
    require(!memory.read(79,2,result)&&result==std::vector<uint8_t>({7,8}),"failed full-range read preserves output");
    require(memory.read(80,0,result)&&result.empty(),"zero-length read at end");
    require(!memory.check(63,0,1)&&!memory.check(81,0,1),"outside zero-length positions rejected");
}
void contexts() {
    self_fixture::numbers("context-actions-u64le",{101,202,63,64,0x1234,999,17,99});
    runtime_core::Contexts states({101,202});
    require(states.id() == 101, "first self context");
    states.setError(17);
    require(states.allocateSlot(63) && states.setSlot(63, 0x1234), "bounded context-local slot");
    require(!states.allocateSlot(63) && !states.allocateSlot(64), "duplicate and out-of-range slots");
    require(states.select(202) && states.error() == 0, "separate context error");
    uint32_t value = 77;
    require(!states.getSlot(63, value) && value == 77, "unallocated slot leaves output unchanged");
    require(!states.select(999) && states.id() == 202, "unknown selection leaves active context unchanged");
    require(states.select(101) && states.error() == 17, "context data preserved");
    require(states.getSlot(63, value) && value == 0x1234 && states.error() == 0, "slot get clears own error");
    require(states.releaseSlot(63) && !states.getSlot(63, value), "released slot rejected");
    require(states.allocateSlot(63) && states.getSlot(63, value) && value == 0, "reallocated slot reset");
    states.setError(99); value=77;
    require(!states.getSlot(64,value)&&value==77&&states.error()==99,"failed get preserves output and error");
    require(!states.setSlot(64,1)&&!states.releaseSlot(64),"invalid slot operations");
    for(const auto& identifiers:std::vector<std::vector<uint32_t>>{{},{0},{1,1},std::vector<uint32_t>(65,1)}) {
        std::vector<uint64_t> values(identifiers.begin(),identifiers.end());
        self_fixture::numbers("context-constructor-u64le",values);
        bool rejected=false;
        try {runtime_core::Contexts invalid(identifiers);} catch(const std::invalid_argument&) {rejected=true;}
        require(rejected,"invalid context constructor rejected");
    }
}
}
int main() {
    try {
        heapAndMemory(); importAndFailureBoundaries(); memoryBoundaries(); contexts();
        std::printf("{\"passed\":true,\"assertions\":%zu,\"input\":\"self-authored\",\"original_engine\":false}\n", assertions);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what()); return 1;
    }
}
