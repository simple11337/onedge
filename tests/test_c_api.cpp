#include "test_harness.h"
#include "onedge/c_api.h"

#include <cstring>
#include <cstdint>

OE_TEST(c_api_error_message) {
    OE_ASSERT(std::strcmp(oe_error_message(OE_OK), "ok") == 0);
    const char* m = oe_error_message(OE_E_OUT_OF_MEMORY);
    OE_ASSERT(m != nullptr);
    OE_ASSERT(std::strstr(m, "memory") != nullptr);
}

OE_TEST(c_api_buffer_lifecycle) {
    oe_buffer* b = nullptr;
    OE_ASSERT_EQ(oe_buffer_create(0, &b), OE_OK);
    OE_ASSERT(b != nullptr);
    OE_ASSERT_EQ(oe_buffer_size(b), std::size_t{0});
    oe_buffer_destroy(b);

    OE_ASSERT_EQ(oe_buffer_create(32, &b), OE_OK);
    OE_ASSERT(b != nullptr);
    OE_ASSERT(oe_buffer_capacity(b) >= 32);
    oe_buffer_destroy(b);

    // Destroy on null is a no-op.
    oe_buffer_destroy(nullptr);
}

OE_TEST(c_api_buffer_null_out_ptr) {
    OE_ASSERT_EQ(oe_buffer_create(0, nullptr), OE_E_NULL_POINTER);
}

OE_TEST(c_api_buffer_append_and_read) {
    oe_buffer* b = nullptr;
    OE_ASSERT_EQ(oe_buffer_create(0, &b), OE_OK);
    OE_ASSERT_EQ(oe_buffer_append(b, "hi", 2), OE_OK);
    OE_ASSERT_EQ(oe_buffer_size(b), std::size_t{2});

    char out[5] = {};
    size_t copied = 0;
    OE_ASSERT_EQ(oe_buffer_read(b, 0, out, sizeof(out), &copied), OE_OK);
    OE_ASSERT_EQ(copied, std::size_t{2});
    OE_ASSERT(std::memcmp(out, "hi", 2) == 0);
    oe_buffer_destroy(b);
}

OE_TEST(c_api_buffer_resize_and_write) {
    oe_buffer* b = nullptr;
    OE_ASSERT_EQ(oe_buffer_create(0, &b), OE_OK);
    OE_ASSERT_EQ(oe_buffer_resize(b, 8, 0xFF), OE_OK);
    OE_ASSERT_EQ(oe_buffer_write(b, 2, "AB", 2), OE_OK);

    const std::uint8_t* p = oe_buffer_data(b);
    OE_ASSERT(p != nullptr);
    OE_ASSERT_EQ(p[0], std::uint8_t{0xFF});
    OE_ASSERT_EQ(p[2], std::uint8_t{'A'});
    OE_ASSERT_EQ(p[3], std::uint8_t{'B'});
    oe_buffer_destroy(b);
}

OE_TEST(c_api_buffer_write_out_of_range) {
    oe_buffer* b = nullptr;
    OE_ASSERT_EQ(oe_buffer_create(0, &b), OE_OK);
    OE_ASSERT_EQ(oe_buffer_resize(b, 4, 0), OE_OK);
    OE_ASSERT_EQ(oe_buffer_write(b, 0, "12345", 5), OE_E_OUT_OF_RANGE);
    oe_buffer_destroy(b);
}

OE_TEST(c_api_buffer_null_handle_returns_error) {
    OE_ASSERT_EQ(oe_buffer_append(nullptr, "x", 1), OE_E_NULL_POINTER);
    OE_ASSERT_EQ(oe_buffer_resize(nullptr, 1, 0), OE_E_NULL_POINTER);
    OE_ASSERT_EQ(oe_buffer_write(nullptr, 0, "x", 1), OE_E_NULL_POINTER);
    OE_ASSERT_EQ(oe_buffer_read(nullptr, 0, nullptr, 0, nullptr), OE_E_NULL_POINTER);
    OE_ASSERT_EQ(oe_buffer_size(nullptr), std::size_t{0});
    OE_ASSERT(oe_buffer_data(nullptr) == nullptr);
}

OE_TEST(c_api_string_lifecycle) {
    oe_string* s = nullptr;
    OE_ASSERT_EQ(oe_string_create(&s), OE_OK);
    OE_ASSERT(s != nullptr);
    OE_ASSERT(std::strcmp(oe_string_cstr(s), "") == 0);
    oe_string_destroy(s);

    oe_string_destroy(nullptr);
}

OE_TEST(c_api_string_from_cstr) {
    oe_string* s = nullptr;
    OE_ASSERT_EQ(oe_string_from_cstr("hello", &s), OE_OK);
    OE_ASSERT(std::strcmp(oe_string_cstr(s), "hello") == 0);
    OE_ASSERT_EQ(oe_string_size(s), std::size_t{5});
    oe_string_destroy(s);
}

OE_TEST(c_api_string_from_cstr_null) {
    oe_string* s = nullptr;
    OE_ASSERT_EQ(oe_string_from_cstr(nullptr, &s), OE_E_NULL_POINTER);
    OE_ASSERT(s == nullptr);
}

OE_TEST(c_api_string_append_cstr_and_bytes) {
    oe_string* s = nullptr;
    OE_ASSERT_EQ(oe_string_create(&s), OE_OK);
    OE_ASSERT_EQ(oe_string_append_cstr(s, "abc"), OE_OK);
    OE_ASSERT_EQ(oe_string_append_bytes(s, "DEF", 3), OE_OK);
    OE_ASSERT(std::strcmp(oe_string_cstr(s), "abcDEF") == 0);
    oe_string_destroy(s);
}

OE_TEST(c_api_string_append_cstr_null) {
    oe_string* s = nullptr;
    OE_ASSERT_EQ(oe_string_create(&s), OE_OK);
    OE_ASSERT_EQ(oe_string_append_cstr(s, nullptr), OE_E_NULL_POINTER);
    oe_string_destroy(s);
}

OE_TEST(c_api_string_copy_to_cstr_truncates) {
    oe_string* s = nullptr;
    OE_ASSERT_EQ(oe_string_from_cstr("toolong", &s), OE_OK);
    char out[4] = {};
    size_t copied = 0;
    OE_ASSERT_EQ(oe_string_copy_to_cstr(s, out, sizeof(out), &copied), OE_E_TRUNCATED);
    OE_ASSERT_EQ(copied, std::size_t{3});
    OE_ASSERT(std::strcmp(out, "too") == 0);
    oe_string_destroy(s);
}

OE_TEST(c_api_string_copy_to_cstr_zero_len) {
    oe_string* s = nullptr;
    OE_ASSERT_EQ(oe_string_from_cstr("x", &s), OE_OK);
    char out[1] = {};
    OE_ASSERT_EQ(oe_string_copy_to_cstr(s, out, 0, nullptr), OE_E_INVALID_ARGUMENT);
    oe_string_destroy(s);
}

OE_TEST(c_api_string_null_handle) {
    OE_ASSERT_EQ(oe_string_append_cstr(nullptr, "x"), OE_E_NULL_POINTER);
    OE_ASSERT_EQ(oe_string_append_bytes(nullptr, "x", 1), OE_E_NULL_POINTER);
    OE_ASSERT_EQ(oe_string_size(nullptr), std::size_t{0});
    OE_ASSERT(std::strcmp(oe_string_cstr(nullptr), "") == 0);
}

OE_TEST(c_api_string_clear) {
    oe_string* s = nullptr;
    OE_ASSERT_EQ(oe_string_from_cstr("data", &s), OE_OK);
    oe_string_clear(s);
    OE_ASSERT_EQ(oe_string_size(s), std::size_t{0});
    OE_ASSERT(std::strcmp(oe_string_cstr(s), "") == 0);
    oe_string_clear(nullptr);
    oe_string_destroy(s);
}

OE_TEST(c_api_buffer_secure_clear) {
    oe_buffer* b = nullptr;
    OE_ASSERT_EQ(oe_buffer_create(0, &b), OE_OK);
    OE_ASSERT_EQ(oe_buffer_append(b, "SECRET", 6), OE_OK);
    const std::uint8_t* snap = oe_buffer_data(b);
    OE_ASSERT(snap != nullptr);
    oe_buffer_secure_clear(b);
    OE_ASSERT_EQ(oe_buffer_size(b), std::size_t{0});
    for (int i = 0; i < 6; ++i) {
        OE_ASSERT_EQ(snap[i], std::uint8_t{0});
    }
    oe_buffer_destroy(b);
    oe_buffer_secure_clear(nullptr);
}

OE_TEST(c_api_buffer_verify) {
    oe_buffer* b = nullptr;
    OE_ASSERT_EQ(oe_buffer_create(16, &b), OE_OK);
    OE_ASSERT_EQ(oe_buffer_verify(b), 1);
    OE_ASSERT_EQ(oe_buffer_append(b, "data", 4), OE_OK);
    OE_ASSERT_EQ(oe_buffer_verify(b), 1);
    oe_buffer_destroy(b);
    OE_ASSERT_EQ(oe_buffer_verify(nullptr), 0);
}

OE_TEST(c_api_string_secure_clear) {
    oe_string* s = nullptr;
    OE_ASSERT_EQ(oe_string_from_cstr("PASSWORD", &s), OE_OK);
    const char* snap = oe_string_cstr(s);
    OE_ASSERT(snap != nullptr);
    oe_string_secure_clear(s);
    OE_ASSERT_EQ(oe_string_size(s), std::size_t{0});
    for (int i = 0; i < 8; ++i) {
        OE_ASSERT_EQ(snap[i], char{0});
    }
    oe_string_destroy(s);
    oe_string_secure_clear(nullptr);
}

OE_TEST(c_api_string_verify) {
    oe_string* s = nullptr;
    OE_ASSERT_EQ(oe_string_create(&s), OE_OK);
    OE_ASSERT_EQ(oe_string_verify(s), 1);
    OE_ASSERT_EQ(oe_string_append_cstr(s, "x"), OE_OK);
    OE_ASSERT_EQ(oe_string_verify(s), 1);
    oe_string_destroy(s);
    OE_ASSERT_EQ(oe_string_verify(nullptr), 0);
}

OE_TEST(c_api_string_size_with_embedded_nul) {
    oe_string* s = nullptr;
    const char data[] = {'a', 0, 'b', 0, 'c'};
    OE_ASSERT_EQ(oe_string_from_bytes(data, 5, &s), OE_OK);
    // size is the full byte length, not strlen.
    OE_ASSERT_EQ(oe_string_size(s), std::size_t{5});
    // The C view sees only up to the first embedded NUL.
    OE_ASSERT(std::strcmp(oe_string_cstr(s), "a") == 0);
    oe_string_destroy(s);
}

OE_TEST(c_api_buffer_resize_to_zero_keeps_capacity) {
    oe_buffer* b = nullptr;
    OE_ASSERT_EQ(oe_buffer_create(0, &b), OE_OK);
    OE_ASSERT_EQ(oe_buffer_resize(b, 32, 0), OE_OK);
    size_t cap_before = oe_buffer_capacity(b);
    OE_ASSERT_EQ(oe_buffer_resize(b, 0, 0), OE_OK);
    OE_ASSERT_EQ(oe_buffer_size(b), std::size_t{0});
    OE_ASSERT_EQ(oe_buffer_capacity(b), cap_before);
    oe_buffer_destroy(b);
}

OE_TEST(c_api_buffer_append_after_secure_clear) {
    oe_buffer* b = nullptr;
    OE_ASSERT_EQ(oe_buffer_create(0, &b), OE_OK);
    OE_ASSERT_EQ(oe_buffer_append(b, "OLD", 3), OE_OK);
    oe_buffer_secure_clear(b);
    OE_ASSERT_EQ(oe_buffer_size(b), std::size_t{0});
    OE_ASSERT_EQ(oe_buffer_append(b, "NEW", 3), OE_OK);
    OE_ASSERT_EQ(oe_buffer_size(b), std::size_t{3});
    const std::uint8_t* p = oe_buffer_data(b);
    OE_ASSERT_EQ(p[0], std::uint8_t{'N'});
    OE_ASSERT_EQ(p[2], std::uint8_t{'W'});
    oe_buffer_destroy(b);
}

OE_TEST(c_api_status_codes_distinct) {
    // Each defined status must round-trip through oe_error_message
    // returning a non-null, non-empty string.
    oe_status codes[] = {OE_OK, OE_E_OUT_OF_MEMORY, OE_E_INVALID_ARGUMENT,
                         OE_E_OUT_OF_RANGE, OE_E_OVERFLOW, OE_E_TRUNCATED,
                         OE_E_NOT_NULL_TERMINATED, OE_E_CAPACITY_EXCEEDED,
                         OE_E_NULL_POINTER, OE_E_UNSUPPORTED, OE_E_PARSE_ERROR};
    for (oe_status c : codes) {
        const char* m = oe_error_message(c);
        OE_ASSERT(m != nullptr);
        OE_ASSERT(m[0] != '\0');
    }
}
