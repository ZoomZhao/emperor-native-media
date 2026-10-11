#include "strict_zlib.h"
#include <algorithm>
#include <climits>
#include <cstring>
#include <new>
#include <vector>
#include <zlib.h>

namespace {
struct InflateSession {
    z_stream stream{};
    bool initialized = false;
    ~InflateSession() { if (initialized) inflateEnd(&stream); }
    int close() {
        initialized = false;
        return inflateEnd(&stream);
    }
};
}

extern "C" emfr_decode_result emfr_decode_zlib(
    const uint8_t *source, size_t source_size, uint8_t *destination,
    size_t destination_capacity, emfr_decode_limits limits) {
    emfr_decode_result result{};
    result.status = EMFR_INVALID_ARGUMENT;
    if ((!source && source_size) || (!destination && destination_capacity)) return result;
    if (source_size > limits.max_encoded_bytes || source_size > UINT_MAX) {
        result.status = EMFR_INPUT_LIMIT;
        return result;
    }
    const size_t output_limit = std::min(destination_capacity, limits.max_decoded_bytes);
    // One private probe byte distinguishes exact-capacity success from overflow.
    if (output_limit >= UINT_MAX) {
        result.status = EMFR_OUTPUT_LIMIT;
        return result;
    }
    try {
        std::vector<uint8_t> scratch(output_limit + 1);
        uint8_t empty_input = 0;
        InflateSession session;
        session.stream.next_in = const_cast<Bytef *>(source_size ? source : &empty_input);
        session.stream.avail_in = static_cast<uInt>(source_size);
        session.stream.next_out = scratch.data();
        session.stream.avail_out = static_cast<uInt>(scratch.size());
        const int init = inflateInit2(&session.stream, 15);
        result.codec_status = init;
        if (init != Z_OK) {
            result.status = init == Z_MEM_ERROR ? EMFR_NO_MEMORY : EMFR_LIBRARY_FAILURE;
            return result;
        }
        session.initialized = true;
        const int code = inflate(&session.stream, Z_FINISH);
        result.codec_status = code;
        // Z_NEED_DICT can return before zlib updates total_in. Remaining input
        // is the authoritative count for this single-call input slice.
        if (session.stream.avail_in > source_size) {
            result.status = EMFR_LIBRARY_FAILURE;
            return result;
        }
        result.input_consumed = source_size - session.stream.avail_in;
        result.output_produced = static_cast<size_t>(session.stream.total_out);
        if (result.input_consumed > source_size || result.output_produced > scratch.size()) {
            result.status = EMFR_LIBRARY_FAILURE;
            return result;
        }
        result.trailing_input = source_size - result.input_consumed;
        if (result.output_produced > output_limit) result.status = EMFR_OUTPUT_LIMIT;
        else if (code == Z_NEED_DICT) result.status = EMFR_DICTIONARY_REQUIRED;
        else if (code == Z_DATA_ERROR) result.status = EMFR_INVALID_DATA;
        else if (code == Z_MEM_ERROR) result.status = EMFR_NO_MEMORY;
        else if (code == Z_BUF_ERROR || code == Z_OK) result.status = EMFR_TRUNCATED;
        else if (code != Z_STREAM_END) result.status = EMFR_LIBRARY_FAILURE;
        else result.status = EMFR_OK;
        const int end = session.close();
        if (result.status == EMFR_OK && end != Z_OK) {
            result.status = EMFR_LIBRARY_FAILURE;
            result.codec_status = end;
        }
        if (result.status == EMFR_OK) {
            if (result.output_produced) std::memcpy(destination, scratch.data(), result.output_produced);
            result.output_available = result.output_produced;
        }
        return result;
    } catch (const std::bad_alloc &) {
        result.status = EMFR_NO_MEMORY;
    } catch (...) {
        result.status = EMFR_LIBRARY_FAILURE;
    }
    return result;
}

extern "C" const char *emfr_status_name(emfr_status status) {
    switch (status) {
    case EMFR_OK: return "ok";
    case EMFR_INVALID_ARGUMENT: return "invalid_argument";
    case EMFR_INPUT_LIMIT: return "input_limit";
    case EMFR_OUTPUT_LIMIT: return "output_limit";
    case EMFR_TRUNCATED: return "truncated";
    case EMFR_DICTIONARY_REQUIRED: return "dictionary_required";
    case EMFR_INVALID_DATA: return "invalid_data";
    case EMFR_NO_MEMORY: return "no_memory";
    case EMFR_LIBRARY_FAILURE: return "library_failure";
    }
    return "unknown_status";
}

extern "C" const char *emfr_zlib_runtime_version(void) { return zlibVersion(); }
