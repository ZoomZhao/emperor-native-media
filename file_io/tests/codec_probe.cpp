#include "strict_zlib.h"
#include <cstdint>

// Test-only symbol indirection measures actual adapter calls. The production
// source is compiled unchanged apart from renaming its public entry symbol.
extern "C" emfr_decode_result emfr_decode_zlib_actual(
    const uint8_t *, size_t, uint8_t *, size_t, emfr_decode_limits);
namespace { uint64_t calls = 0; }
extern "C" uint64_t emfr_test_codec_calls(void) { return calls; }
extern "C" emfr_decode_result emfr_decode_zlib(
    const uint8_t *source, size_t size, uint8_t *destination, size_t capacity,
    emfr_decode_limits limits) {
    ++calls;
    return emfr_decode_zlib_actual(source, size, destination, capacity, limits);
}
