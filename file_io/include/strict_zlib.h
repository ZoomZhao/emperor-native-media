#ifndef EMPEROR_FILE_IO_STRICT_ZLIB_H
#define EMPEROR_FILE_IO_STRICT_ZLIB_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum emfr_status {
    EMFR_OK = 0,
    EMFR_INVALID_ARGUMENT,
    EMFR_INPUT_LIMIT,
    EMFR_OUTPUT_LIMIT,
    EMFR_TRUNCATED,
    EMFR_DICTIONARY_REQUIRED,
    EMFR_INVALID_DATA,
    EMFR_NO_MEMORY,
    EMFR_LIBRARY_FAILURE
} emfr_status;

typedef struct emfr_decode_limits {
    size_t max_encoded_bytes;
    size_t max_decoded_bytes;
} emfr_decode_limits;

typedef struct emfr_decode_result {
    emfr_status status;
    int codec_status;
    size_t input_consumed;
    size_t output_produced; /* Diagnostic, including a private failed prefix. */
    size_t output_available; /* Zero unless status == EMFR_OK. */
    size_t trailing_input;
} emfr_decode_result;

/* Decode one zlib-wrapped stream (+15), with no dictionary or gzip/raw fallback.
 * Caller supplies valid input/output slices and trusted resource limits.
 * Output is transactional: any failure leaves destination bytes unchanged.
 * Trailing bytes inside this input slice are reported, not decoded as members.
 * Zero-capacity output is allowed for a valid empty stream.
 */
emfr_decode_result emfr_decode_zlib(const uint8_t *source, size_t source_size,
                                 uint8_t *destination, size_t destination_capacity,
                                 emfr_decode_limits limits);
const char *emfr_status_name(emfr_status status);
const char *emfr_zlib_runtime_version(void);

#ifdef __cplusplus
}
#endif
#endif
