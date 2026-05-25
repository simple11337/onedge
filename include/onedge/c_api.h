#ifndef ONEDGE_C_API_H
#define ONEDGE_C_API_H

#include "onedge/onedge_config.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Numeric values mirror oe::error_code; static_asserts in the .cpp.
typedef enum oe_status {
    OE_OK                       = 0,
    OE_E_OUT_OF_MEMORY          = 1,
    OE_E_INVALID_ARGUMENT       = 2,
    OE_E_OUT_OF_RANGE           = 3,
    OE_E_OVERFLOW               = 4,
    OE_E_TRUNCATED              = 5,
    OE_E_NOT_NULL_TERMINATED    = 6,
    OE_E_CAPACITY_EXCEEDED      = 7,
    OE_E_NULL_POINTER           = 8,
    OE_E_UNSUPPORTED            = 9,
    OE_E_PARSE_ERROR            = 10
} oe_status;

ONEDGE_API const char* oe_error_message(oe_status code);

// Buffer
typedef struct oe_buffer oe_buffer;

ONEDGE_API oe_status oe_buffer_create(size_t initial_capacity, oe_buffer** out);
ONEDGE_API void      oe_buffer_destroy(oe_buffer* b);  // null-safe

ONEDGE_API size_t oe_buffer_size(const oe_buffer* b);
ONEDGE_API size_t oe_buffer_capacity(const oe_buffer* b);

// Invalidated by anything that may grow.
ONEDGE_API const uint8_t* oe_buffer_data(const oe_buffer* b);

ONEDGE_API oe_status oe_buffer_reserve(oe_buffer* b, size_t new_cap);
ONEDGE_API oe_status oe_buffer_resize(oe_buffer* b, size_t new_size, uint8_t fill_byte);
ONEDGE_API void      oe_buffer_clear(oe_buffer* b);
ONEDGE_API void      oe_buffer_secure_clear(oe_buffer* b);
ONEDGE_API int       oe_buffer_verify(const oe_buffer* b);

ONEDGE_API oe_status oe_buffer_append(oe_buffer* b, const void* data, size_t len);

// *out_copied may be less than len when the read runs past the end.
ONEDGE_API oe_status oe_buffer_read(const oe_buffer* b,
                                    size_t offset,
                                    void* out,
                                    size_t len,
                                    size_t* out_copied);

ONEDGE_API oe_status oe_buffer_write(oe_buffer* b,
                                     size_t offset,
                                     const void* src,
                                     size_t len);

// String
typedef struct oe_string oe_string;

ONEDGE_API oe_status oe_string_create(oe_string** out);
ONEDGE_API oe_status oe_string_from_cstr(const char* s, oe_string** out);
ONEDGE_API oe_status oe_string_from_bytes(const char* data, size_t len, oe_string** out);

ONEDGE_API void   oe_string_destroy(oe_string* s);
ONEDGE_API size_t oe_string_size(const oe_string* s);
ONEDGE_API size_t oe_string_capacity(const oe_string* s);

// Always NUL-terminated. Returns "" for null/empty.
ONEDGE_API const char* oe_string_cstr(const oe_string* s);

ONEDGE_API oe_status oe_string_reserve(oe_string* s, size_t new_cap);
ONEDGE_API void      oe_string_clear(oe_string* s);
ONEDGE_API void      oe_string_secure_clear(oe_string* s);
ONEDGE_API int       oe_string_verify(const oe_string* s);

ONEDGE_API oe_status oe_string_append_cstr(oe_string* s, const char* cstr);
ONEDGE_API oe_status oe_string_append_bytes(oe_string* s, const char* data, size_t len);

// out_len == 0 returns OE_E_INVALID_ARGUMENT. OE_E_TRUNCATED when full
// string didn't fit; *out_copied is what was written before the NUL.
ONEDGE_API oe_status oe_string_copy_to_cstr(const oe_string* s,
                                            char* out,
                                            size_t out_len,
                                            size_t* out_copied);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // ONEDGE_C_API_H
