#include "test_harness.h"
#include "onedge/checked_math.h"

#include <cstdint>
#include <limits>

using oe::checked_add;
using oe::checked_sub;
using oe::checked_mul;
using oe::checked_grow;
using oe::range_in_bounds;
using oe::checked_cast;
using oe::safe_add;
using oe::safe_mul;
using oe::error_code;

constexpr std::size_t kMax = std::numeric_limits<std::size_t>::max();

OE_TEST(checked_add_basic) {
    OE_ASSERT_EQ(checked_add(0, 0).value(), std::size_t{0});
    OE_ASSERT_EQ(checked_add(1, 2).value(), std::size_t{3});
    OE_ASSERT_EQ(checked_add(kMax, 0).value(), kMax);
    OE_ASSERT_EQ(checked_add(0, kMax).value(), kMax);
}

OE_TEST(checked_add_overflow) {
    OE_ASSERT(!checked_add(kMax, 1).has_value());
    OE_ASSERT(!checked_add(1, kMax).has_value());
    OE_ASSERT(!checked_add(kMax, kMax).has_value());
    OE_ASSERT(!checked_add(kMax - 1, 2).has_value());
}

OE_TEST(checked_add_boundary) {
    OE_ASSERT_EQ(checked_add(kMax - 1, 1).value(), kMax);
}

OE_TEST(checked_sub_basic) {
    OE_ASSERT_EQ(checked_sub(5, 3).value(), std::size_t{2});
    OE_ASSERT_EQ(checked_sub(0, 0).value(), std::size_t{0});
    OE_ASSERT_EQ(checked_sub(kMax, kMax).value(), std::size_t{0});
}

OE_TEST(checked_sub_underflow) {
    OE_ASSERT(!checked_sub(0, 1).has_value());
    OE_ASSERT(!checked_sub(3, 5).has_value());
}

OE_TEST(checked_mul_basic) {
    OE_ASSERT_EQ(checked_mul(0, 0).value(), std::size_t{0});
    OE_ASSERT_EQ(checked_mul(0, kMax).value(), std::size_t{0});
    OE_ASSERT_EQ(checked_mul(kMax, 0).value(), std::size_t{0});
    OE_ASSERT_EQ(checked_mul(1, kMax).value(), kMax);
    OE_ASSERT_EQ(checked_mul(2, 3).value(), std::size_t{6});
}

OE_TEST(checked_mul_overflow) {
    OE_ASSERT(!checked_mul(kMax, 2).has_value());
    OE_ASSERT(!checked_mul(2, kMax).has_value());
    OE_ASSERT(!checked_mul(kMax / 2 + 1, 2).has_value());
}

OE_TEST(checked_mul_boundary) {
    OE_ASSERT_EQ(checked_mul(kMax / 2, 2).value(), (kMax / 2) * 2);
}

OE_TEST(checked_grow_cap) {
    OE_ASSERT_EQ(checked_grow(0, 100, 1024).value(), std::size_t{100});
    OE_ASSERT_EQ(checked_grow(1000, 24, 1024).value(), std::size_t{1024});
    OE_ASSERT(!checked_grow(1000, 25, 1024).has_value());
    OE_ASSERT(!checked_grow(kMax, 1, 1024).has_value());
}

OE_TEST(range_in_bounds_normal) {
    OE_ASSERT(range_in_bounds(0, 0, 0));
    OE_ASSERT(range_in_bounds(0, 0, 10));
    OE_ASSERT(range_in_bounds(0, 10, 10));
    OE_ASSERT(range_in_bounds(5, 5, 10));
    OE_ASSERT(range_in_bounds(10, 0, 10));
    OE_ASSERT(!range_in_bounds(0, 11, 10));
    OE_ASSERT(!range_in_bounds(5, 6, 10));
    OE_ASSERT(!range_in_bounds(11, 0, 10));
}

OE_TEST(range_in_bounds_overflow) {
    OE_ASSERT(!range_in_bounds(kMax, 1, kMax));
    OE_ASSERT(!range_in_bounds(1, kMax, kMax));
}

OE_TEST(checked_cast_same_signedness) {
    auto v = checked_cast<std::int32_t>(std::int64_t{42});
    OE_ASSERT_EQ(v.value(), std::int32_t{42});

    auto under = checked_cast<std::int32_t>(std::int64_t{-1} * (std::int64_t{1} << 40));
    OE_ASSERT(!under.has_value());

    auto over = checked_cast<std::int32_t>(std::int64_t{1} << 40);
    OE_ASSERT(!over.has_value());
}

OE_TEST(checked_cast_signed_to_unsigned) {
    auto v = checked_cast<std::uint32_t>(std::int32_t{42});
    OE_ASSERT_EQ(v.value(), std::uint32_t{42});

    auto neg = checked_cast<std::uint32_t>(std::int32_t{-1});
    OE_ASSERT(!neg.has_value());
}

OE_TEST(checked_cast_unsigned_to_signed) {
    auto v = checked_cast<std::int32_t>(std::uint32_t{42});
    OE_ASSERT_EQ(v.value(), std::int32_t{42});

    auto too_big = checked_cast<std::int32_t>(std::uint32_t{0x80000000u});
    OE_ASSERT(!too_big.has_value());
}

OE_TEST(safe_add_returns_overflow_code) {
    auto r = safe_add(kMax, 1);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::overflow);
}

OE_TEST(safe_mul_returns_overflow_code) {
    auto r = safe_mul(kMax, 2);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::overflow);
}

OE_TEST(checked_mul_size_max_times_one_is_size_max) {
    OE_ASSERT_EQ(checked_mul(kMax, 1).value(), kMax);
    OE_ASSERT_EQ(checked_mul(1, kMax).value(), kMax);
}

OE_TEST(checked_grow_zero_additional_is_current) {
    OE_ASSERT_EQ(checked_grow(100, 0, 1024).value(), std::size_t{100});
    OE_ASSERT_EQ(checked_grow(0, 0, 1024).value(), std::size_t{0});
}

OE_TEST(checked_grow_current_at_cap_zero_extra) {
    OE_ASSERT_EQ(checked_grow(1024, 0, 1024).value(), std::size_t{1024});
}

OE_TEST(checked_cast_identity_keeps_value) {
    auto a = checked_cast<std::size_t>(std::size_t{0});
    OE_ASSERT_EQ(a.value(), std::size_t{0});
    auto b = checked_cast<std::size_t>(kMax);
    OE_ASSERT_EQ(b.value(), kMax);
}

OE_TEST(checked_cast_smallest_signed_negative) {
    auto v = checked_cast<std::uint8_t>(std::int8_t{-1});
    OE_ASSERT(!v.has_value());
}

OE_TEST(range_in_bounds_offset_at_size_zero_len) {
    OE_ASSERT(range_in_bounds(10, 0, 10));   // offset == size, len 0 is legal
    OE_ASSERT(!range_in_bounds(11, 0, 10));  // offset past size, len 0 is not
}
