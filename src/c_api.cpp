#include "onedge/c_api.h"
#include "onedge/buffer.h"
#include "onedge/string.h"
#include "onedge/error.h"

#include <new>
#include <utility>

// Lockstep check; reorder one without the other and the build fails.
static_assert(static_cast<int>(oe::error_code::ok)                  == OE_OK,                       "");
static_assert(static_cast<int>(oe::error_code::out_of_memory)       == OE_E_OUT_OF_MEMORY,          "");
static_assert(static_cast<int>(oe::error_code::invalid_argument)    == OE_E_INVALID_ARGUMENT,       "");
static_assert(static_cast<int>(oe::error_code::out_of_range)        == OE_E_OUT_OF_RANGE,           "");
static_assert(static_cast<int>(oe::error_code::overflow)            == OE_E_OVERFLOW,               "");
static_assert(static_cast<int>(oe::error_code::truncated)           == OE_E_TRUNCATED,              "");
static_assert(static_cast<int>(oe::error_code::not_null_terminated) == OE_E_NOT_NULL_TERMINATED,    "");
static_assert(static_cast<int>(oe::error_code::capacity_exceeded)   == OE_E_CAPACITY_EXCEEDED,      "");
static_assert(static_cast<int>(oe::error_code::null_pointer)        == OE_E_NULL_POINTER,           "");
static_assert(static_cast<int>(oe::error_code::unsupported)         == OE_E_UNSUPPORTED,            "");
static_assert(static_cast<int>(oe::error_code::parse_error)         == OE_E_PARSE_ERROR,            "");

struct oe_buffer { oe::buffer impl; };
struct oe_string { oe::string impl; };

namespace {
oe_status to_status(oe::error_code c) noexcept {
    return static_cast<oe_status>(static_cast<int>(c));
}
}  // namespace

extern "C" {

const char* oe_error_message(oe_status code) {
    return oe::error_message(static_cast<oe::error_code>(static_cast<int>(code)));
}

// ---------------------------------------------------------------------------
// Buffer
// ---------------------------------------------------------------------------

oe_status oe_buffer_create(size_t initial_capacity, oe_buffer** out) {
    if (!out) return OE_E_NULL_POINTER;
    *out = nullptr;
    auto r = oe::buffer::with_capacity(initial_capacity);
    if (!r.ok()) return to_status(r.code());
    auto* wrapper = new (std::nothrow) oe_buffer{ std::move(r).value() };
    if (!wrapper) return OE_E_OUT_OF_MEMORY;
    *out = wrapper;
    return OE_OK;
}

void oe_buffer_destroy(oe_buffer* b) {
    delete b;
}

size_t oe_buffer_size(const oe_buffer* b) {
    return b ? b->impl.size() : 0;
}

size_t oe_buffer_capacity(const oe_buffer* b) {
    return b ? b->impl.capacity() : 0;
}

const uint8_t* oe_buffer_data(const oe_buffer* b) {
    return b ? b->impl.data() : nullptr;
}

oe_status oe_buffer_reserve(oe_buffer* b, size_t new_cap) {
    if (!b) return OE_E_NULL_POINTER;
    auto r = b->impl.reserve(new_cap);
    return to_status(r.code());
}

oe_status oe_buffer_resize(oe_buffer* b, size_t new_size, uint8_t fill_byte) {
    if (!b) return OE_E_NULL_POINTER;
    auto r = b->impl.resize(new_size, fill_byte);
    return to_status(r.code());
}

void oe_buffer_clear(oe_buffer* b) {
    if (b) b->impl.clear();
}

void oe_buffer_secure_clear(oe_buffer* b) {
    if (b) b->impl.secure_clear();
}

int oe_buffer_verify(const oe_buffer* b) {
    return b && b->impl.verify() ? 1 : 0;
}

oe_status oe_buffer_append(oe_buffer* b, const void* data, size_t len) {
    if (!b) return OE_E_NULL_POINTER;
    auto r = b->impl.append(data, len);
    return to_status(r.code());
}

oe_status oe_buffer_read(const oe_buffer* b,
                         size_t offset,
                         void* out,
                         size_t len,
                         size_t* out_copied) {
    if (!b) return OE_E_NULL_POINTER;
    auto r = b->impl.read(offset, out, len);
    if (!r.ok()) {
        if (out_copied) *out_copied = 0;
        return to_status(r.code());
    }
    if (out_copied) *out_copied = r.value();
    return OE_OK;
}

oe_status oe_buffer_write(oe_buffer* b,
                          size_t offset,
                          const void* src,
                          size_t len) {
    if (!b) return OE_E_NULL_POINTER;
    auto r = b->impl.write(offset, src, len);
    return to_status(r.code());
}

// ---------------------------------------------------------------------------
// String
// ---------------------------------------------------------------------------

oe_status oe_string_create(oe_string** out) {
    if (!out) return OE_E_NULL_POINTER;
    *out = nullptr;
    auto* wrapper = new (std::nothrow) oe_string{};
    if (!wrapper) return OE_E_OUT_OF_MEMORY;
    *out = wrapper;
    return OE_OK;
}

oe_status oe_string_from_cstr(const char* s, oe_string** out) {
    if (!out) return OE_E_NULL_POINTER;
    *out = nullptr;
    auto r = oe::string::from_cstr(s);
    if (!r.ok()) return to_status(r.code());
    auto* wrapper = new (std::nothrow) oe_string{ std::move(r).value() };
    if (!wrapper) return OE_E_OUT_OF_MEMORY;
    *out = wrapper;
    return OE_OK;
}

oe_status oe_string_from_bytes(const char* data, size_t len, oe_string** out) {
    if (!out) return OE_E_NULL_POINTER;
    *out = nullptr;
    auto r = oe::string::from_bytes(data, len);
    if (!r.ok()) return to_status(r.code());
    auto* wrapper = new (std::nothrow) oe_string{ std::move(r).value() };
    if (!wrapper) return OE_E_OUT_OF_MEMORY;
    *out = wrapper;
    return OE_OK;
}

void oe_string_destroy(oe_string* s) {
    delete s;
}

size_t oe_string_size(const oe_string* s) {
    return s ? s->impl.size() : 0;
}

size_t oe_string_capacity(const oe_string* s) {
    return s ? s->impl.capacity() : 0;
}

const char* oe_string_cstr(const oe_string* s) {
    return s ? s->impl.c_str() : "";
}

oe_status oe_string_reserve(oe_string* s, size_t new_cap) {
    if (!s) return OE_E_NULL_POINTER;
    auto r = s->impl.reserve(new_cap);
    return to_status(r.code());
}

void oe_string_clear(oe_string* s) {
    if (s) s->impl.clear();
}

void oe_string_secure_clear(oe_string* s) {
    if (s) s->impl.secure_clear();
}

int oe_string_verify(const oe_string* s) {
    return s && s->impl.verify() ? 1 : 0;
}

oe_status oe_string_append_cstr(oe_string* s, const char* cstr) {
    if (!s) return OE_E_NULL_POINTER;
    if (!cstr) return OE_E_NULL_POINTER;
    // bounded scan. could use strnlen on POSIX but it's not in C99 and
    // MSVC has its own thing. this loop is fine.
    size_t i = 0;
    while (i < ONEDGE_MAX_ALLOC_BYTES && cstr[i] != '\0') ++i;
    if (i == ONEDGE_MAX_ALLOC_BYTES && cstr[i - 1] != '\0') {
        return OE_E_NOT_NULL_TERMINATED;
    }
    auto r = s->impl.append(cstr, i);
    return to_status(r.code());
}

oe_status oe_string_append_bytes(oe_string* s, const char* data, size_t len) {
    if (!s) return OE_E_NULL_POINTER;
    auto r = s->impl.append(data, len);
    return to_status(r.code());
}

oe_status oe_string_copy_to_cstr(const oe_string* s,
                                 char* out,
                                 size_t out_len,
                                 size_t* out_copied) {
    if (!s) return OE_E_NULL_POINTER;
    auto r = s->impl.copy_to_cstr(out, out_len);
    if (!r.ok()) {
        // On truncation the partial count isn't in result<>, recompute it.
        if (r.code() == oe::error_code::truncated && out_copied && out_len > 0) {
            size_t available = s->impl.size();
            size_t written   = (available < out_len - 1) ? available : (out_len - 1);
            *out_copied = written;
        } else if (out_copied) {
            *out_copied = 0;
        }
        return to_status(r.code());
    }
    if (out_copied) *out_copied = r.value();
    return OE_OK;
}

}  // extern "C"
