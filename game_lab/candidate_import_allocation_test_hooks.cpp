// Linked only into the candidate test executable, never the App/provider.
#include <cstdlib>
#include <new>
#include <cstddef>
namespace candidate_allocation_test {
bool armed = false;
size_t failures = 0;
void arm() { armed = true; }
void disarm() { armed = false; }
}
void* operator new(std::size_t size) {
    // The controller's input/reserve sizes differ. This exact allocation is
    // made inside FramedReader::next for its fixed decoding buffer.
    if (candidate_allocation_test::armed && size == 36144) {
        candidate_allocation_test::armed = false;
        ++candidate_allocation_test::failures;
        throw std::bad_alloc();
    }
    if (void* p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
