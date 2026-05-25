#include "test_harness.h"
#include "onedge/reader.h"

#include <cstring>
#include <cstdint>

using oe::reader;
using oe::const_byte_span;
using oe::error_code;

OE_TEST(reader_default_empty) {
    reader r;
    OE_ASSERT(r.empty());
    OE_ASSERT_EQ(r.position(), std::size_t{0});
    OE_ASSERT_EQ(r.remaining(), std::size_t{0});
}

OE_TEST(reader_from_pointer_size) {
    const std::uint8_t data[] = {0xAB, 0xCD};
    reader r(data, 2);
    OE_ASSERT(!r.empty());
    OE_ASSERT_EQ(r.remaining(), std::size_t{2});
    auto v = r.read_u8();
    OE_ASSERT_OK(v);
    OE_ASSERT_EQ(v.value(), std::uint8_t{0xAB});
    OE_ASSERT_EQ(r.position(), std::size_t{1});
}

OE_TEST(reader_read_u8_at_end_fails_without_advance) {
    const std::uint8_t data[] = {0x01};
    reader r(data, 1);
    auto a = r.read_u8();
    OE_ASSERT_OK(a);
    OE_ASSERT_EQ(r.position(), std::size_t{1});
    auto b = r.read_u8();
    OE_ASSERT(!b.ok());
    OE_ASSERT_EQ(b.code(), error_code::out_of_range);
    OE_ASSERT_EQ(r.position(), std::size_t{1});  // not advanced
}

OE_TEST(reader_read_u16_be) {
    const std::uint8_t data[] = {0x12, 0x34};
    reader r(data, 2);
    auto v = r.read_u16_be();
    OE_ASSERT_OK(v);
    OE_ASSERT_EQ(v.value(), std::uint16_t{0x1234});
}

OE_TEST(reader_read_u32_be) {
    const std::uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    reader r(data, 4);
    auto v = r.read_u32_be();
    OE_ASSERT_OK(v);
    OE_ASSERT_EQ(v.value(), std::uint32_t{0xDEADBEEF});
}

OE_TEST(reader_read_u32_le) {
    const std::uint8_t data[] = {0xEF, 0xBE, 0xAD, 0xDE};
    reader r(data, 4);
    auto v = r.read_u32_le();
    OE_ASSERT_OK(v);
    OE_ASSERT_EQ(v.value(), std::uint32_t{0xDEADBEEF});
}

OE_TEST(reader_read_u64_be) {
    const std::uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    reader r(data, 8);
    auto v = r.read_u64_be();
    OE_ASSERT_OK(v);
    OE_ASSERT_EQ(v.value(), std::uint64_t{0x0102030405060708ULL});
}

OE_TEST(reader_short_u32_fails_without_advance) {
    const std::uint8_t data[] = {0x01, 0x02, 0x03};
    reader r(data, 3);
    auto v = r.read_u32_be();
    OE_ASSERT(!v.ok());
    OE_ASSERT_EQ(v.code(), error_code::out_of_range);
    OE_ASSERT_EQ(r.position(), std::size_t{0});
}

OE_TEST(reader_skip_advances) {
    const std::uint8_t data[] = {1, 2, 3, 4, 5};
    reader r(data, 5);
    OE_ASSERT_OK(r.skip(2));
    OE_ASSERT_EQ(r.position(), std::size_t{2});
    auto v = r.read_u8();
    OE_ASSERT_OK(v);
    OE_ASSERT_EQ(v.value(), std::uint8_t{3});
}

OE_TEST(reader_skip_past_end_fails) {
    const std::uint8_t data[] = {1, 2};
    reader r(data, 2);
    auto s = r.skip(3);
    OE_ASSERT(!s.ok());
    OE_ASSERT_EQ(s.code(), error_code::out_of_range);
    OE_ASSERT_EQ(r.position(), std::size_t{0});
}

OE_TEST(reader_read_bytes_basic) {
    const std::uint8_t data[] = {'a', 'b', 'c', 'd'};
    reader r(data, 4);
    char out[3] = {};
    OE_ASSERT_OK(r.read_bytes(out, 3));
    OE_ASSERT(std::memcmp(out, "abc", 3) == 0);
    OE_ASSERT_EQ(r.position(), std::size_t{3});
}

OE_TEST(reader_read_bytes_too_many_fails) {
    const std::uint8_t data[] = {1, 2};
    reader r(data, 2);
    char out[5] = {};
    auto x = r.read_bytes(out, 5);
    OE_ASSERT(!x.ok());
    OE_ASSERT_EQ(x.code(), error_code::out_of_range);
    OE_ASSERT_EQ(r.position(), std::size_t{0});
}

OE_TEST(reader_read_bytes_null_nonzero) {
    const std::uint8_t data[] = {1, 2};
    reader r(data, 2);
    auto x = r.read_bytes(nullptr, 1);
    OE_ASSERT(!x.ok());
    OE_ASSERT_EQ(x.code(), error_code::null_pointer);
}

OE_TEST(reader_read_bytes_zero_len_noop) {
    const std::uint8_t data[] = {1};
    reader r(data, 1);
    OE_ASSERT_OK(r.read_bytes(nullptr, 0));
    OE_ASSERT_EQ(r.position(), std::size_t{0});
}

OE_TEST(reader_read_view_borrows) {
    const std::uint8_t data[] = {1, 2, 3, 4};
    reader r(data, 4);
    auto v = r.read_view(2);
    OE_ASSERT_OK(v);
    OE_ASSERT_EQ(v.value().size(), std::size_t{2});
    OE_ASSERT_EQ(v.value()[0], std::uint8_t{1});
    OE_ASSERT_EQ(v.value()[1], std::uint8_t{2});
    OE_ASSERT_EQ(r.position(), std::size_t{2});
}

OE_TEST(reader_peek_view_does_not_advance) {
    const std::uint8_t data[] = {1, 2, 3};
    reader r(data, 3);
    auto v = r.peek_view(2);
    OE_ASSERT_OK(v);
    OE_ASSERT_EQ(r.position(), std::size_t{0});
    OE_ASSERT_EQ(v.value()[0], std::uint8_t{1});
}

OE_TEST(reader_peek_view_past_end_fails) {
    const std::uint8_t data[] = {1};
    reader r(data, 1);
    auto v = r.peek_view(2);
    OE_ASSERT(!v.ok());
    OE_ASSERT_EQ(v.code(), error_code::out_of_range);
}

OE_TEST(reader_chained_reads_consume_in_order) {
    // length-prefixed string: u8 len, then bytes
    const std::uint8_t data[] = {3, 'F', 'O', 'O', 0xCA, 0xFE};
    reader r(data, sizeof(data));
    auto len = r.read_u8();
    OE_ASSERT_OK(len);
    OE_ASSERT_EQ(len.value(), std::uint8_t{3});
    auto body = r.read_view(len.value());
    OE_ASSERT_OK(body);
    OE_ASSERT_EQ(body.value().size(), std::size_t{3});
    OE_ASSERT_EQ(body.value()[0], std::uint8_t{'F'});
    auto trailer = r.read_u16_be();
    OE_ASSERT_OK(trailer);
    OE_ASSERT_EQ(trailer.value(), std::uint16_t{0xCAFE});
    OE_ASSERT(r.empty());
}

OE_TEST(reader_remaining_after_partial) {
    const std::uint8_t data[] = {1, 2, 3, 4, 5};
    reader r(data, 5);
    OE_ASSERT_OK(r.skip(2));
    OE_ASSERT_EQ(r.remaining(), std::size_t{3});
}
