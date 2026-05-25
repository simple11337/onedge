#include "test_harness.h"
#include "onedge/buffer.h"

#include <cstdint>
#include <cstring>
#include <limits>

using oe::buffer;
using oe::error_code;

OE_TEST(buffer_default_empty) {
    buffer b;
    OE_ASSERT(b.empty());
    OE_ASSERT_EQ(b.size(), std::size_t{0});
    OE_ASSERT_EQ(b.capacity(), std::size_t{0});
    OE_ASSERT(b.data() == nullptr);
}

OE_TEST(buffer_with_capacity_zero) {
    auto r = buffer::with_capacity(0);
    OE_ASSERT_OK(r);
    OE_ASSERT_EQ(r.value().capacity(), std::size_t{0});
}

OE_TEST(buffer_with_capacity_normal) {
    auto r = buffer::with_capacity(32);
    OE_ASSERT_OK(r);
    OE_ASSERT(r.value().capacity() >= 32);
    OE_ASSERT_EQ(r.value().size(), std::size_t{0});
}

OE_TEST(buffer_with_capacity_overflow_cap) {
    auto r = buffer::with_capacity(std::numeric_limits<std::size_t>::max());
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::capacity_exceeded);
}

OE_TEST(buffer_from_bytes_normal) {
    const std::uint8_t data[] = {1, 2, 3, 4, 5};
    auto r = buffer::from_bytes(data, sizeof(data));
    OE_ASSERT_OK(r);
    OE_ASSERT_EQ(r.value().size(), sizeof(data));
    OE_ASSERT(std::memcmp(r.value().data(), data, sizeof(data)) == 0);
}

OE_TEST(buffer_from_bytes_zero_len) {
    auto r = buffer::from_bytes(nullptr, 0);
    OE_ASSERT_OK(r);
    OE_ASSERT_EQ(r.value().size(), std::size_t{0});
}

OE_TEST(buffer_from_bytes_null_with_nonzero_len) {
    auto r = buffer::from_bytes(nullptr, 5);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::null_pointer);
}

OE_TEST(buffer_from_bytes_with_zero_bytes) {
    const std::uint8_t data[] = {0, 'A', 0, 'B', 0};
    auto r = buffer::from_bytes(data, sizeof(data));
    OE_ASSERT_OK(r);
    OE_ASSERT_EQ(r.value().size(), sizeof(data));
    OE_ASSERT_EQ(r.value().at(0).value(), std::uint8_t{0});
    OE_ASSERT_EQ(r.value().at(1).value(), std::uint8_t{'A'});
    OE_ASSERT_EQ(r.value().at(4).value(), std::uint8_t{0});
}

OE_TEST(buffer_append_grows) {
    buffer b;
    for (int i = 0; i < 100; ++i) {
        OE_ASSERT_OK(b.append(static_cast<std::uint8_t>(i)));
    }
    OE_ASSERT_EQ(b.size(), std::size_t{100});
    for (int i = 0; i < 100; ++i) {
        OE_ASSERT_EQ(b.at(static_cast<std::size_t>(i)).value(),
                     static_cast<std::uint8_t>(i));
    }
}

OE_TEST(buffer_append_bytes_zero_len) {
    buffer b;
    OE_ASSERT_OK(b.append(nullptr, 0));
    OE_ASSERT_EQ(b.size(), std::size_t{0});
}

OE_TEST(buffer_append_bytes_null_nonzero) {
    buffer b;
    auto r = b.append(nullptr, 1);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::null_pointer);
}

OE_TEST(buffer_append_overflow) {
    buffer b;
    // Push size up via reserve and then try to add SIZE_MAX bytes.
    auto r = b.append(reinterpret_cast<const void*>(1), std::numeric_limits<std::size_t>::max());
    // First catch should be either overflow or capacity_exceeded depending
    // on whether we got past the size add. Either is acceptable - both
    // mean we refused the unsafe operation.
    OE_ASSERT(!r.ok());
    OE_ASSERT(r.code() == error_code::overflow ||
              r.code() == error_code::capacity_exceeded);
}

OE_TEST(buffer_at_bounds) {
    buffer b;
    OE_ASSERT_OK(b.append(static_cast<std::uint8_t>(7)));
    OE_ASSERT_EQ(b.at(0).value(), std::uint8_t{7});
    auto r = b.at(1);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::out_of_range);
}

OE_TEST(buffer_at_empty_always_out_of_range) {
    buffer b;
    auto r = b.at(0);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::out_of_range);
}

OE_TEST(buffer_set_in_bounds) {
    auto rb = buffer::from_bytes("abc", 3);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    OE_ASSERT_OK(b.set(1, 'Z'));
    OE_ASSERT_EQ(b.at(1).value(), std::uint8_t{'Z'});
}

OE_TEST(buffer_set_out_of_bounds) {
    buffer b;
    auto r = b.set(0, 1);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::out_of_range);
}

OE_TEST(buffer_resize_grow_fills) {
    buffer b;
    OE_ASSERT_OK(b.resize(5, 0xCC));
    OE_ASSERT_EQ(b.size(), std::size_t{5});
    for (std::size_t i = 0; i < 5; ++i) {
        OE_ASSERT_EQ(b.at(i).value(), std::uint8_t{0xCC});
    }
}

OE_TEST(buffer_resize_shrink_preserves) {
    auto rb = buffer::from_bytes("abcdef", 6);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    OE_ASSERT_OK(b.resize(3));
    OE_ASSERT_EQ(b.size(), std::size_t{3});
    OE_ASSERT_EQ(b.at(0).value(), std::uint8_t{'a'});
    OE_ASSERT_EQ(b.at(1).value(), std::uint8_t{'b'});
    OE_ASSERT_EQ(b.at(2).value(), std::uint8_t{'c'});
}

OE_TEST(buffer_resize_to_zero) {
    auto rb = buffer::from_bytes("xyz", 3);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    OE_ASSERT_OK(b.resize(0));
    OE_ASSERT_EQ(b.size(), std::size_t{0});
    OE_ASSERT(b.capacity() >= 3);  // capacity preserved
}

OE_TEST(buffer_resize_capacity_cap) {
    buffer b;
    auto r = b.resize(std::numeric_limits<std::size_t>::max() - 1);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::capacity_exceeded);
}

OE_TEST(buffer_write_in_bounds) {
    buffer b;
    OE_ASSERT_OK(b.resize(10, 0));
    const std::uint8_t data[3] = {'a', 'b', 'c'};
    OE_ASSERT_OK(b.write(2, data, 3));
    OE_ASSERT_EQ(b.at(2).value(), std::uint8_t{'a'});
    OE_ASSERT_EQ(b.at(3).value(), std::uint8_t{'b'});
    OE_ASSERT_EQ(b.at(4).value(), std::uint8_t{'c'});
}

OE_TEST(buffer_write_exact_fit) {
    buffer b;
    OE_ASSERT_OK(b.resize(4, 0));
    const std::uint8_t data[4] = {1, 2, 3, 4};
    OE_ASSERT_OK(b.write(0, data, 4));
}

OE_TEST(buffer_write_one_byte_too_large) {
    buffer b;
    OE_ASSERT_OK(b.resize(4, 0));
    const std::uint8_t data[5] = {1, 2, 3, 4, 5};
    auto r = b.write(0, data, 5);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::out_of_range);
}

OE_TEST(buffer_write_offset_past_end) {
    buffer b;
    OE_ASSERT_OK(b.resize(4, 0));
    auto r = b.write(5, "x", 1);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::out_of_range);
}

OE_TEST(buffer_write_offset_overflow) {
    buffer b;
    OE_ASSERT_OK(b.resize(4, 0));
    auto r = b.write(std::numeric_limits<std::size_t>::max(), "x", 2);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::overflow);
}

OE_TEST(buffer_read_basic) {
    auto rb = buffer::from_bytes("hello", 5);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    char out[10] = {};
    auto r = b.read(0, out, 5);
    OE_ASSERT_OK(r);
    OE_ASSERT_EQ(r.value(), std::size_t{5});
    OE_ASSERT(std::memcmp(out, "hello", 5) == 0);
}

OE_TEST(buffer_read_partial_at_end) {
    auto rb = buffer::from_bytes("abc", 3);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    char out[10] = {};
    auto r = b.read(1, out, 100);
    OE_ASSERT_OK(r);
    OE_ASSERT_EQ(r.value(), std::size_t{2});  // only 'b','c' available
}

OE_TEST(buffer_read_offset_equals_size) {
    auto rb = buffer::from_bytes("abc", 3);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    char out[1] = {};
    auto r = b.read(3, out, 1);
    OE_ASSERT_OK(r);
    OE_ASSERT_EQ(r.value(), std::size_t{0});
}

OE_TEST(buffer_read_offset_past_end) {
    auto rb = buffer::from_bytes("abc", 3);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    char out[1] = {};
    auto r = b.read(4, out, 1);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::out_of_range);
}

OE_TEST(buffer_clone_independent) {
    auto rb = buffer::from_bytes("abc", 3);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    auto cloned = b.clone();
    OE_ASSERT_OK(cloned);
    OE_ASSERT_OK(b.set(0, 'X'));
    OE_ASSERT_EQ(cloned.value().at(0).value(), std::uint8_t{'a'});
    OE_ASSERT_EQ(b.at(0).value(), std::uint8_t{'X'});
}

OE_TEST(buffer_move_clears_source) {
    auto rb = buffer::from_bytes("data", 4);
    OE_ASSERT_OK(rb);
    auto a = std::move(rb).value();
    auto b = std::move(a);
    OE_ASSERT_EQ(a.size(), std::size_t{0});
    OE_ASSERT(a.data() == nullptr);
    OE_ASSERT_EQ(b.size(), std::size_t{4});
}

OE_TEST(buffer_fill) {
    buffer b;
    OE_ASSERT_OK(b.resize(8, 0));
    b.fill(0xAB);
    for (std::size_t i = 0; i < 8; ++i) {
        OE_ASSERT_EQ(b.at(i).value(), std::uint8_t{0xAB});
    }
}

OE_TEST(buffer_clear_keeps_capacity) {
    auto rb = buffer::from_bytes("abcd", 4);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    std::size_t cap_before = b.capacity();
    b.clear();
    OE_ASSERT_EQ(b.size(), std::size_t{0});
    OE_ASSERT_EQ(b.capacity(), cap_before);
}

OE_TEST(buffer_self_append_handles_realloc) {
    auto rb = buffer::with_capacity(4);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    OE_ASSERT_OK(b.append("ABCD", 4));
    // Append the buffer to itself. cap is 4 so this forces a grow,
    // which would invalidate a naive src pointer.
    OE_ASSERT_OK(b.append(b.data(), b.size()));
    OE_ASSERT_EQ(b.size(), std::size_t{8});
    OE_ASSERT_EQ(b.at(0).value(), std::uint8_t{'A'});
    OE_ASSERT_EQ(b.at(3).value(), std::uint8_t{'D'});
    OE_ASSERT_EQ(b.at(4).value(), std::uint8_t{'A'});
    OE_ASSERT_EQ(b.at(7).value(), std::uint8_t{'D'});
}

OE_TEST(buffer_self_append_no_realloc) {
    auto rb = buffer::with_capacity(64);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    OE_ASSERT_OK(b.append("XYZW", 4));
    OE_ASSERT_OK(b.append(b.data(), b.size()));
    OE_ASSERT_EQ(b.size(), std::size_t{8});
    OE_ASSERT_EQ(b.at(0).value(), std::uint8_t{'X'});
    OE_ASSERT_EQ(b.at(7).value(), std::uint8_t{'W'});
}

OE_TEST(buffer_write_overlapping_forward) {
    auto rb = buffer::from_bytes("ABCDEF", 6);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    OE_ASSERT_OK(b.write(2, b.data(), 4));
    OE_ASSERT_EQ(b.at(2).value(), std::uint8_t{'A'});
    OE_ASSERT_EQ(b.at(3).value(), std::uint8_t{'B'});
    OE_ASSERT_EQ(b.at(4).value(), std::uint8_t{'C'});
    OE_ASSERT_EQ(b.at(5).value(), std::uint8_t{'D'});
}

OE_TEST(buffer_write_overlapping_backward) {
    auto rb = buffer::from_bytes("ABCDEF", 6);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    OE_ASSERT_OK(b.write(0, b.data() + 2, 4));
    OE_ASSERT_EQ(b.at(0).value(), std::uint8_t{'C'});
    OE_ASSERT_EQ(b.at(3).value(), std::uint8_t{'F'});
}

OE_TEST(buffer_secure_clear_zeroes) {
    const char secret[] = "TOPSECRET-creds";
    auto rb = buffer::from_bytes(secret, sizeof(secret) - 1);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    const std::uint8_t* snap = b.data();
    std::size_t old_size = b.size();
    b.secure_clear();
    OE_ASSERT(b.empty());
    // Capacity preserved means snap is still valid; bytes should be 0.
    for (std::size_t i = 0; i < old_size; ++i) {
        OE_ASSERT_EQ(snap[i], std::uint8_t{0});
    }
}

OE_TEST(buffer_verify_default) {
    buffer b;
    OE_ASSERT(b.verify());
}

OE_TEST(buffer_verify_through_lifecycle) {
    auto rb = buffer::from_bytes("abc", 3);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    OE_ASSERT(b.verify());
    OE_ASSERT_OK(b.append('X'));
    OE_ASSERT(b.verify());
    OE_ASSERT_OK(b.resize(10, 0));
    OE_ASSERT(b.verify());
    b.clear();
    OE_ASSERT(b.verify());
    b.secure_clear();
    OE_ASSERT(b.verify());
}

OE_TEST(buffer_clone_does_not_alias) {
    auto rb = buffer::from_bytes("abcd", 4);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    auto cl = b.clone();
    OE_ASSERT_OK(cl);
    auto c = std::move(cl).value();
    OE_ASSERT(b.data() != c.data());
}

OE_TEST(buffer_self_move_assign_safe) {
    auto rb = buffer::from_bytes("payload", 7);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    // Self move-assign must not double-free or wipe state.
    b = std::move(b);
    OE_ASSERT_EQ(b.size(), std::size_t{7});
    OE_ASSERT_EQ(b.at(0).value(), std::uint8_t{'p'});
    OE_ASSERT_EQ(b.at(6).value(), std::uint8_t{'d'});
    OE_ASSERT(b.verify());
}

OE_TEST(buffer_at_size_max_returns_oor) {
    buffer b;
    OE_ASSERT_OK(b.append('x'));
    auto r = b.at(std::numeric_limits<std::size_t>::max());
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::out_of_range);
}

OE_TEST(buffer_append_byte_at_exact_capacity) {
    auto rb = buffer::with_capacity(4);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    OE_ASSERT_OK(b.append("ABCD", 4));  // exactly fills capacity
    OE_ASSERT_EQ(b.size(), b.capacity());
    OE_ASSERT_OK(b.append('E'));        // must grow
    OE_ASSERT_EQ(b.size(), std::size_t{5});
    OE_ASSERT(b.capacity() >= 5);
    OE_ASSERT_EQ(b.at(4).value(), std::uint8_t{'E'});
}

OE_TEST(buffer_resize_zero_then_grow_keeps_capacity_path) {
    auto rb = buffer::with_capacity(32);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    OE_ASSERT_OK(b.resize(20, 0xAB));
    std::size_t cap_before = b.capacity();
    OE_ASSERT_OK(b.resize(0));
    OE_ASSERT_EQ(b.capacity(), cap_before);
    OE_ASSERT_OK(b.resize(10, 0xCD));
    for (std::size_t i = 0; i < 10; ++i) {
        OE_ASSERT_EQ(b.at(i).value(), std::uint8_t{0xCD});
    }
}

OE_TEST(buffer_resize_with_nonzero_fill_pattern) {
    buffer b;
    OE_ASSERT_OK(b.resize(4, 0x11));
    OE_ASSERT_OK(b.resize(2));        // shrink (does NOT touch the leftover slots)
    OE_ASSERT_OK(b.resize(6, 0x22));  // grow: new bytes get 0x22, NOT 0x11
    OE_ASSERT_EQ(b.at(0).value(), std::uint8_t{0x11});
    OE_ASSERT_EQ(b.at(1).value(), std::uint8_t{0x11});
    OE_ASSERT_EQ(b.at(2).value(), std::uint8_t{0x22});
    OE_ASSERT_EQ(b.at(5).value(), std::uint8_t{0x22});
}

OE_TEST(buffer_secure_clear_idempotent) {
    auto rb = buffer::from_bytes("data", 4);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    b.secure_clear();
    b.secure_clear();
    b.secure_clear();
    OE_ASSERT(b.empty());
    OE_ASSERT(b.verify());
}

OE_TEST(buffer_append_after_secure_clear_reusable) {
    auto rb = buffer::from_bytes("OLDSECRET", 9);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    b.secure_clear();
    OE_ASSERT_OK(b.append("NEW", 3));
    OE_ASSERT_EQ(b.size(), std::size_t{3});
    OE_ASSERT_EQ(b.at(0).value(), std::uint8_t{'N'});
    OE_ASSERT(b.verify());
}

OE_TEST(buffer_write_offset_at_size_zero_len_noop) {
    auto rb = buffer::from_bytes("abc", 3);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    // offset == size + len == 0 is a degenerate but legal range.
    OE_ASSERT_OK(b.write(b.size(), nullptr, 0));
    OE_ASSERT_EQ(b.size(), std::size_t{3});
}

OE_TEST(buffer_from_bytes_single_byte) {
    const std::uint8_t one = 0x7F;
    auto rb = buffer::from_bytes(&one, 1);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    OE_ASSERT_EQ(b.size(), std::size_t{1});
    OE_ASSERT_EQ(b.at(0).value(), std::uint8_t{0x7F});
}

OE_TEST(buffer_from_bytes_single_nul) {
    const std::uint8_t z = 0;
    auto rb = buffer::from_bytes(&z, 1);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    OE_ASSERT_EQ(b.size(), std::size_t{1});
    OE_ASSERT_EQ(b.at(0).value(), std::uint8_t{0});
}

OE_TEST(buffer_clone_preserves_binary_content) {
    const std::uint8_t raw[] = {0x00, 0xFF, 0x7F, 0x80, 0x00, 0x01};
    auto rb = buffer::from_bytes(raw, sizeof(raw));
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    auto cl = b.clone();
    OE_ASSERT_OK(cl);
    auto c = std::move(cl).value();
    OE_ASSERT_EQ(c.size(), sizeof(raw));
    for (std::size_t i = 0; i < sizeof(raw); ++i) {
        OE_ASSERT_EQ(c.at(i).value(), raw[i]);
    }
}

OE_TEST(buffer_read_with_offset_size_max_returns_oor) {
    auto rb = buffer::from_bytes("abc", 3);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    char out[1] = {};
    auto r = b.read(std::numeric_limits<std::size_t>::max(), out, 1);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::out_of_range);
}

OE_TEST(buffer_reserve_to_same_capacity_is_noop) {
    auto rb = buffer::with_capacity(64);
    OE_ASSERT_OK(rb);
    auto b = std::move(rb).value();
    const auto* before = b.data();
    OE_ASSERT_OK(b.reserve(64));
    OE_ASSERT_OK(b.reserve(32));  // smaller -> no shrink, no realloc
    OE_ASSERT(b.data() == before);
}
