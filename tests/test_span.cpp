#include "test_harness.h"
#include "onedge/span.h"

#include <cstdint>

using oe::span;
using oe::byte_span;
using oe::const_byte_span;
using oe::error_code;

OE_TEST(span_default_empty) {
    span<int> s;
    OE_ASSERT(s.empty());
    OE_ASSERT_EQ(s.size(), std::size_t{0});
    OE_ASSERT(s.data() == nullptr);
}

OE_TEST(span_from_pointer_and_size) {
    int arr[3] = {1, 2, 3};
    span<int> s(arr, 3);
    OE_ASSERT_EQ(s.size(), std::size_t{3});
    OE_ASSERT_EQ(s[0], 1);
    OE_ASSERT_EQ(s[2], 3);
}

OE_TEST(span_from_c_array) {
    int arr[4] = {10, 20, 30, 40};
    span<int> s(arr);
    OE_ASSERT_EQ(s.size(), std::size_t{4});
    OE_ASSERT_EQ(s[0], 10);
    OE_ASSERT_EQ(s[3], 40);
}

OE_TEST(span_mutable_to_const_conversion) {
    int arr[2] = {7, 8};
    span<int> mut(arr);
    span<const int> cs = mut;
    OE_ASSERT_EQ(cs.size(), std::size_t{2});
    OE_ASSERT_EQ(cs[0], 7);
}

OE_TEST(span_in_bounds) {
    int arr[3] = {0, 0, 0};
    span<int> s(arr);
    OE_ASSERT(s.in_bounds(0));
    OE_ASSERT(s.in_bounds(2));
    OE_ASSERT(!s.in_bounds(3));
    OE_ASSERT(!s.in_bounds(std::numeric_limits<std::size_t>::max()));
}

OE_TEST(span_at_returns_ptr_or_null) {
    int arr[2] = {5, 6};
    span<int> s(arr);
    OE_ASSERT(s.at(0) != nullptr);
    OE_ASSERT_EQ(*s.at(0), 5);
    OE_ASSERT(s.at(2) == nullptr);
    OE_ASSERT(s.at(std::numeric_limits<std::size_t>::max()) == nullptr);
}

OE_TEST(span_subspan_basic) {
    int arr[5] = {1, 2, 3, 4, 5};
    span<int> s(arr);
    auto sub = s.subspan(1, 3);
    OE_ASSERT_OK(sub);
    OE_ASSERT_EQ(sub.value().size(), std::size_t{3});
    OE_ASSERT_EQ(sub.value()[0], 2);
    OE_ASSERT_EQ(sub.value()[2], 4);
}

OE_TEST(span_subspan_exact_end_zero_len_ok) {
    int arr[3] = {1, 2, 3};
    span<int> s(arr);
    auto sub = s.subspan(3, 0);
    OE_ASSERT_OK(sub);
    OE_ASSERT(sub.value().empty());
}

OE_TEST(span_subspan_past_end_fails) {
    int arr[3] = {1, 2, 3};
    span<int> s(arr);
    auto sub = s.subspan(2, 5);
    OE_ASSERT(!sub.ok());
    OE_ASSERT_EQ(sub.code(), error_code::out_of_range);
}

OE_TEST(span_subspan_offset_past_end_fails) {
    int arr[3] = {1, 2, 3};
    span<int> s(arr);
    auto sub = s.subspan(4, 0);
    OE_ASSERT(!sub.ok());
    OE_ASSERT_EQ(sub.code(), error_code::out_of_range);
}

OE_TEST(span_first_caps_when_too_large) {
    int arr[3] = {1, 2, 3};
    span<int> s(arr);
    auto f = s.first(10);
    OE_ASSERT_EQ(f.size(), std::size_t{3});  // capped, not error
    auto g = s.first(2);
    OE_ASSERT_EQ(g.size(), std::size_t{2});
    OE_ASSERT_EQ(g[0], 1);
    OE_ASSERT_EQ(g[1], 2);
}

OE_TEST(span_last_caps_when_too_large) {
    int arr[3] = {1, 2, 3};
    span<int> s(arr);
    auto l = s.last(10);
    OE_ASSERT_EQ(l.size(), std::size_t{3});
    auto m = s.last(2);
    OE_ASSERT_EQ(m.size(), std::size_t{2});
    OE_ASSERT_EQ(m[0], 2);
    OE_ASSERT_EQ(m[1], 3);
}

OE_TEST(span_iteration) {
    int arr[4] = {1, 2, 3, 4};
    span<int> s(arr);
    int sum = 0;
    for (int v : s) sum += v;
    OE_ASSERT_EQ(sum, 10);
}

OE_TEST(span_byte_span_size_bytes) {
    std::uint8_t arr[5] = {0, 1, 2, 3, 4};
    byte_span s(arr);
    OE_ASSERT_EQ(s.size_bytes(), std::size_t{5});
}

OE_TEST(span_as_bytes_of_int_span) {
    std::uint32_t arr[2] = {0xAABBCCDD, 0x11223344};
    span<std::uint32_t> s(arr);
    const_byte_span bs = oe::as_bytes(s);
    OE_ASSERT_EQ(bs.size(), std::size_t{8});
}

OE_TEST(span_subspan_overflow_offset_plus_count) {
    int arr[3] = {1, 2, 3};
    span<int> s(arr);
    auto sub = s.subspan(2, std::numeric_limits<std::size_t>::max());
    OE_ASSERT(!sub.ok());
    OE_ASSERT_EQ(sub.code(), error_code::out_of_range);
}
