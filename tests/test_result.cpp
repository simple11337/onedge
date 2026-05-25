#include "test_harness.h"
#include "onedge/result.h"

#include <string>

using oe::result;
using oe::error_code;

OE_TEST(result_int_success) {
    auto r = result<int>::success(42);
    OE_ASSERT(r.ok());
    OE_ASSERT(static_cast<bool>(r));
    OE_ASSERT_EQ(r.value(), 42);
    OE_ASSERT_EQ(r.code(), error_code::ok);
}

OE_TEST(result_int_failure) {
    auto r = result<int>::failure(error_code::out_of_memory);
    OE_ASSERT(!r.ok());
    OE_ASSERT(!static_cast<bool>(r));
    OE_ASSERT_EQ(r.code(), error_code::out_of_memory);
    OE_ASSERT(std::string(r.message()).find("memory") != std::string::npos);
}

OE_TEST(result_value_or) {
    auto good = result<int>::success(7);
    auto bad  = result<int>::failure(error_code::overflow);
    OE_ASSERT_EQ(good.value_or(0), 7);
    OE_ASSERT_EQ(bad.value_or(99), 99);
}

OE_TEST(result_move_semantics) {
    auto r1 = result<std::string>::success(std::string("hello"));
    OE_ASSERT(r1.ok());
    auto r2 = std::move(r1);
    OE_ASSERT(r2.ok());
    OE_ASSERT_EQ(r2.value(), std::string("hello"));
}

OE_TEST(result_copy_semantics) {
    auto r1 = result<std::string>::success(std::string("xyz"));
    auto r2 = r1;
    OE_ASSERT(r1.ok());
    OE_ASSERT(r2.ok());
    OE_ASSERT_EQ(r1.value(), std::string("xyz"));
    OE_ASSERT_EQ(r2.value(), std::string("xyz"));
}

OE_TEST(result_void_success) {
    auto r = result<void>::success();
    OE_ASSERT(r.ok());
    OE_ASSERT_EQ(r.code(), error_code::ok);
}

OE_TEST(result_void_failure) {
    auto r = result<void>::failure(error_code::invalid_argument, "bad");
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::invalid_argument);
    OE_ASSERT_EQ(std::string(r.message()), std::string("bad"));
}

OE_TEST(result_custom_message) {
    auto r = result<int>::failure(error_code::parse_error, "expected digit");
    OE_ASSERT_EQ(std::string(r.message()), std::string("expected digit"));
}

OE_TEST(result_destroys_value_on_assign) {
    // Ensure the held value is destroyed when overwritten. Use a string
    // so the leak would show under ASAN.
    auto r = result<std::string>::success(std::string(64, 'x'));
    OE_ASSERT(r.ok());
    r = result<std::string>::failure(error_code::out_of_memory);
    OE_ASSERT(!r.ok());
    r = result<std::string>::success(std::string(64, 'y'));
    OE_ASSERT(r.ok());
    OE_ASSERT_EQ(r.value().size(), std::size_t{64});
}
