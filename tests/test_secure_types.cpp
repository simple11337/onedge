#include "test_harness.h"
#include "onedge/secure_buffer.h"
#include "onedge/secure_string.h"

#include <cstdint>
#include <cstring>

using oe::secure_buffer;
using oe::secure_string;
using oe::error_code;

OE_TEST(secure_buffer_basic_lifecycle) {
    auto r = secure_buffer::from_bytes("hi", 2);
    OE_ASSERT_OK(r);
    auto sb = std::move(r).value();
    OE_ASSERT_EQ(sb.size(), std::size_t{2});
    OE_ASSERT(sb.data() != nullptr);
}

OE_TEST(secure_buffer_clear_zeros_through_secure_path) {
    auto r = secure_buffer::from_bytes("SECRET", 6);
    OE_ASSERT_OK(r);
    auto sb = std::move(r).value();
    const auto* snap = sb.data();
    std::size_t old = sb.size();
    sb.clear();  // secure_clear is what runs
    OE_ASSERT_EQ(sb.size(), std::size_t{0});
    for (std::size_t i = 0; i < old; ++i) {
        OE_ASSERT_EQ(snap[i], std::uint8_t{0});
    }
}

OE_TEST(secure_buffer_move_assign_takes_over) {
    auto a = secure_buffer::from_bytes("AAAA", 4);
    OE_ASSERT_OK(a);
    auto sa = std::move(a).value();

    auto b = secure_buffer::from_bytes("BBBB", 4);
    OE_ASSERT_OK(b);
    auto sb = std::move(b).value();

    sa = std::move(sb);
    OE_ASSERT_EQ(sa.size(), std::size_t{4});
    OE_ASSERT_EQ(sa.data()[0], std::uint8_t{'B'});
    OE_ASSERT_EQ(sb.size(), std::size_t{0});  // moved-from
    // The zero-before-free behavior is verified by reading the code
    // (secure_clear runs inside operator=); reading the freed block to
    // check would be UB and ASAN would catch us.
}

OE_TEST(secure_buffer_append_and_read) {
    auto r = secure_buffer::with_capacity(0);
    OE_ASSERT_OK(r);
    auto sb = std::move(r).value();
    OE_ASSERT_OK(sb.append("data", 4));
    char out[5] = {};
    auto cp = sb.read(0, out, 4);
    OE_ASSERT_OK(cp);
    OE_ASSERT_EQ(cp.value(), std::size_t{4});
    OE_ASSERT(std::memcmp(out, "data", 4) == 0);
}

OE_TEST(secure_buffer_inner_exposes_full_api) {
    auto r = secure_buffer::with_capacity(0);
    OE_ASSERT_OK(r);
    auto sb = std::move(r).value();
    OE_ASSERT_OK(sb.inner().append('X'));
    OE_ASSERT_EQ(sb.size(), std::size_t{1});
}

OE_TEST(secure_string_basic) {
    auto r = secure_string::from_cstr("password");
    OE_ASSERT_OK(r);
    auto ss = std::move(r).value();
    OE_ASSERT_EQ(ss.size(), std::size_t{8});
    OE_ASSERT(std::strcmp(ss.c_str(), "password") == 0);
}

OE_TEST(secure_string_clear_zeros_via_secure_path) {
    auto r = secure_string::from_cstr("TOPSECRET");
    OE_ASSERT_OK(r);
    auto ss = std::move(r).value();
    const char* snap = ss.c_str();
    std::size_t old = ss.size();
    ss.clear();
    OE_ASSERT(ss.empty());
    for (std::size_t i = 0; i < old; ++i) {
        OE_ASSERT_EQ(snap[i], char{0});
    }
}

OE_TEST(secure_string_move_assign_takes_over) {
    auto a = secure_string::from_cstr("AAAA");
    OE_ASSERT_OK(a);
    auto sa = std::move(a).value();

    auto b = secure_string::from_cstr("BBBB");
    OE_ASSERT_OK(b);
    auto sb = std::move(b).value();

    sa = std::move(sb);
    OE_ASSERT_EQ(sa.size(), std::size_t{4});
    OE_ASSERT(std::strcmp(sa.c_str(), "BBBB") == 0);
    OE_ASSERT_EQ(sb.size(), std::size_t{0});
}

OE_TEST(secure_string_append_grows) {
    auto r = secure_string::from_cstr("a");
    OE_ASSERT_OK(r);
    auto ss = std::move(r).value();
    for (int i = 0; i < 5; ++i) OE_ASSERT_OK(ss.append('a'));
    OE_ASSERT_EQ(ss.size(), std::size_t{6});
}

OE_TEST(secure_string_copy_to_cstr_truncates) {
    auto r = secure_string::from_cstr("longerstring");
    OE_ASSERT_OK(r);
    auto ss = std::move(r).value();
    char out[5] = {};
    auto cp = ss.copy_to_cstr(out, 5);
    OE_ASSERT(!cp.ok());
    OE_ASSERT_EQ(cp.code(), error_code::truncated);
    OE_ASSERT(std::strcmp(out, "long") == 0);
}
