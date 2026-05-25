#include "test_harness.h"
#include "onedge/fixed_buffer.h"

#include <cstdint>
#include <cstring>

using oe::fixed_buffer;
using oe::error_code;

OE_TEST(fixed_buffer_default_empty) {
    fixed_buffer<32> fb;
    OE_ASSERT(fb.empty());
    OE_ASSERT_EQ(fb.size(), std::size_t{0});
    OE_ASSERT_EQ(fb.capacity(), std::size_t{32});
}

OE_TEST(fixed_buffer_append_byte_until_full) {
    fixed_buffer<4> fb;
    for (int i = 0; i < 4; ++i) OE_ASSERT_OK(fb.append(static_cast<std::uint8_t>(i)));
    OE_ASSERT_EQ(fb.size(), std::size_t{4});
    auto r = fb.append(std::uint8_t{0xFF});
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::capacity_exceeded);
}

OE_TEST(fixed_buffer_append_bytes_fits_exactly) {
    fixed_buffer<8> fb;
    OE_ASSERT_OK(fb.append("ABCDEFGH", 8));
    OE_ASSERT_EQ(fb.size(), std::size_t{8});
    OE_ASSERT_EQ(fb.at(0).value(), std::uint8_t{'A'});
    OE_ASSERT_EQ(fb.at(7).value(), std::uint8_t{'H'});
}

OE_TEST(fixed_buffer_append_bytes_one_too_many) {
    fixed_buffer<4> fb;
    auto r = fb.append("12345", 5);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::capacity_exceeded);
    OE_ASSERT_EQ(fb.size(), std::size_t{0});  // nothing committed
}

OE_TEST(fixed_buffer_append_bytes_overflow) {
    fixed_buffer<16> fb;
    OE_ASSERT_OK(fb.append("ab", 2));
    auto r = fb.append(reinterpret_cast<const void*>(1),
                       std::numeric_limits<std::size_t>::max());
    OE_ASSERT(!r.ok());
    OE_ASSERT(r.code() == error_code::overflow ||
              r.code() == error_code::capacity_exceeded);
}

OE_TEST(fixed_buffer_at_out_of_range) {
    fixed_buffer<4> fb;
    OE_ASSERT_OK(fb.append('x'));
    OE_ASSERT_EQ(fb.at(0).value(), std::uint8_t{'x'});
    auto r = fb.at(1);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::out_of_range);
}

OE_TEST(fixed_buffer_set_bounds) {
    fixed_buffer<4> fb;
    OE_ASSERT_OK(fb.append("abc", 3));
    OE_ASSERT_OK(fb.set(1, 'Z'));
    OE_ASSERT_EQ(fb.at(1).value(), std::uint8_t{'Z'});
    auto r = fb.set(3, 0);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::out_of_range);
}

OE_TEST(fixed_buffer_resize_grow_and_shrink) {
    fixed_buffer<10> fb;
    OE_ASSERT_OK(fb.resize(5, 0xAB));
    for (int i = 0; i < 5; ++i) OE_ASSERT_EQ(fb.at(static_cast<std::size_t>(i)).value(), std::uint8_t{0xAB});
    OE_ASSERT_OK(fb.resize(2));
    OE_ASSERT_EQ(fb.size(), std::size_t{2});
}

OE_TEST(fixed_buffer_resize_too_big) {
    fixed_buffer<4> fb;
    auto r = fb.resize(5);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::capacity_exceeded);
}

OE_TEST(fixed_buffer_clear_and_secure_clear) {
    fixed_buffer<8> fb;
    OE_ASSERT_OK(fb.append("SECRET", 6));
    fb.secure_clear();
    OE_ASSERT(fb.empty());
    // secure_clear zeros all N bytes; check the storage past the used region too.
    for (std::size_t i = 0; i < fb.capacity(); ++i) {
        OE_ASSERT_EQ(fb.data()[i], std::uint8_t{0});
    }
}

OE_TEST(fixed_buffer_as_span_reflects_size) {
    fixed_buffer<8> fb;
    OE_ASSERT_OK(fb.append("abcd", 4));
    auto s = fb.as_span();
    OE_ASSERT_EQ(s.size(), std::size_t{4});
    OE_ASSERT_EQ(s[0], std::uint8_t{'a'});
}

OE_TEST(fixed_buffer_verify_through_ops) {
    fixed_buffer<8> fb;
    OE_ASSERT(fb.verify());
    OE_ASSERT_OK(fb.append("hi", 2));
    OE_ASSERT(fb.verify());
    OE_ASSERT_OK(fb.resize(8, 0));
    OE_ASSERT(fb.verify());
    fb.secure_clear();
    OE_ASSERT(fb.verify());
}
