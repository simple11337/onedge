#include "test_harness.h"
#include "onedge/writer.h"
#include "onedge/reader.h"
#include "onedge/buffer.h"

#include <cstdint>

using oe::buffer;
using oe::writer;
using oe::reader;
using oe::error_code;

OE_TEST(writer_default_has_no_target) {
    writer w;
    auto r = w.write_u8(1);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::null_pointer);
}

OE_TEST(writer_write_u8) {
    buffer b;
    writer w(b);
    OE_ASSERT_OK(w.write_u8(0x42));
    OE_ASSERT_EQ(w.written(), std::size_t{1});
    OE_ASSERT_EQ(b.size(), std::size_t{1});
    OE_ASSERT_EQ(b.at(0).value(), std::uint8_t{0x42});
}

OE_TEST(writer_u16_be_then_reader_roundtrips) {
    buffer b;
    writer w(b);
    OE_ASSERT_OK(w.write_u16_be(0xBEEF));
    reader r(b.data(), b.size());
    auto v = r.read_u16_be();
    OE_ASSERT_OK(v);
    OE_ASSERT_EQ(v.value(), std::uint16_t{0xBEEF});
}

OE_TEST(writer_u32_be_byte_order_correct) {
    buffer b;
    writer w(b);
    OE_ASSERT_OK(w.write_u32_be(0x11223344));
    OE_ASSERT_EQ(b.at(0).value(), std::uint8_t{0x11});
    OE_ASSERT_EQ(b.at(1).value(), std::uint8_t{0x22});
    OE_ASSERT_EQ(b.at(2).value(), std::uint8_t{0x33});
    OE_ASSERT_EQ(b.at(3).value(), std::uint8_t{0x44});
}

OE_TEST(writer_u32_le_byte_order_correct) {
    buffer b;
    writer w(b);
    OE_ASSERT_OK(w.write_u32_le(0x11223344));
    OE_ASSERT_EQ(b.at(0).value(), std::uint8_t{0x44});
    OE_ASSERT_EQ(b.at(3).value(), std::uint8_t{0x11});
}

OE_TEST(writer_u64_be_roundtrip) {
    buffer b;
    writer w(b);
    OE_ASSERT_OK(w.write_u64_be(0x0102030405060708ULL));
    reader r(b.data(), b.size());
    auto v = r.read_u64_be();
    OE_ASSERT_OK(v);
    OE_ASSERT_EQ(v.value(), std::uint64_t{0x0102030405060708ULL});
}

OE_TEST(writer_chained_message_build) {
    buffer b;
    writer w(b);
    OE_ASSERT_OK(w.write_u8(0xAA));
    OE_ASSERT_OK(w.write_u16_be(0x0103));
    OE_ASSERT_OK(w.write_bytes("data", 4));
    OE_ASSERT_EQ(w.written(), std::size_t{7});
    OE_ASSERT_EQ(b.size(), std::size_t{7});

    reader r(b.data(), b.size());
    auto h = r.read_u8();
    OE_ASSERT_OK(h);
    OE_ASSERT_EQ(h.value(), std::uint8_t{0xAA});
    auto t = r.read_u16_be();
    OE_ASSERT_OK(t);
    OE_ASSERT_EQ(t.value(), std::uint16_t{0x0103});
    auto body = r.read_view(4);
    OE_ASSERT_OK(body);
    OE_ASSERT_EQ(body.value()[0], std::uint8_t{'d'});
}

OE_TEST(writer_bytes_null_nonzero) {
    buffer b;
    writer w(b);
    auto r = w.write_bytes(nullptr, 1);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::null_pointer);
}
