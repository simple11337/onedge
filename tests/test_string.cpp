#include "test_harness.h"
#include "onedge/string.h"

#include <cstring>
#include <limits>
#include <string_view>

using oe::string;
using oe::error_code;

OE_TEST(string_default_empty) {
    string s;
    OE_ASSERT(s.empty());
    OE_ASSERT_EQ(s.size(), std::size_t{0});
    OE_ASSERT_EQ(s.capacity(), std::size_t{0});
    OE_ASSERT(std::strcmp(s.c_str(), "") == 0);
}

OE_TEST(string_from_cstr_normal) {
    auto r = string::from_cstr("hello");
    OE_ASSERT_OK(r);
    OE_ASSERT_EQ(r.value().size(), std::size_t{5});
    OE_ASSERT(std::strcmp(r.value().c_str(), "hello") == 0);
}

OE_TEST(string_from_cstr_empty) {
    auto r = string::from_cstr("");
    OE_ASSERT_OK(r);
    OE_ASSERT(r.value().empty());
    OE_ASSERT(std::strcmp(r.value().c_str(), "") == 0);
}

OE_TEST(string_from_cstr_null) {
    auto r = string::from_cstr(nullptr);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::null_pointer);
}

OE_TEST(string_from_bytes_with_zero_in_middle) {
    const char data[] = {'a', 0, 'b'};
    auto r = string::from_bytes(data, 3);
    OE_ASSERT_OK(r);
    OE_ASSERT_EQ(r.value().size(), std::size_t{3});
    OE_ASSERT_EQ(r.value().view().size(), std::size_t{3});
    OE_ASSERT_EQ(r.value().at(1).value(), char{0});
    OE_ASSERT_EQ(r.value().at(2).value(), char{'b'});
}

OE_TEST(string_from_bytes_zero_len) {
    auto r = string::from_bytes(nullptr, 0);
    OE_ASSERT_OK(r);
    OE_ASSERT(r.value().empty());
}

OE_TEST(string_from_bytes_null_nonzero) {
    auto r = string::from_bytes(nullptr, 3);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::null_pointer);
}

OE_TEST(string_append_char) {
    string s;
    for (char c = 'a'; c <= 'z'; ++c) {
        OE_ASSERT_OK(s.append(c));
    }
    OE_ASSERT_EQ(s.size(), std::size_t{26});
    OE_ASSERT(std::strcmp(s.c_str(), "abcdefghijklmnopqrstuvwxyz") == 0);
}

OE_TEST(string_append_bytes) {
    string s;
    OE_ASSERT_OK(s.append("foo", 3));
    OE_ASSERT_OK(s.append("bar", 3));
    OE_ASSERT_EQ(s.size(), std::size_t{6});
    OE_ASSERT(std::strcmp(s.c_str(), "foobar") == 0);
}

OE_TEST(string_append_view) {
    string s;
    OE_ASSERT_OK(s.append(std::string_view("hello")));
    OE_ASSERT(std::strcmp(s.c_str(), "hello") == 0);
}

OE_TEST(string_append_empty) {
    auto r = string::from_cstr("base");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.append("", 0));
    OE_ASSERT_OK(s.append(std::string_view{}));
    OE_ASSERT_EQ(s.size(), std::size_t{4});
}

OE_TEST(string_append_null_nonzero) {
    string s;
    auto r = s.append(nullptr, 3);
    OE_ASSERT(!r.ok());
    OE_ASSERT_EQ(r.code(), error_code::null_pointer);
}

OE_TEST(string_append_overflow_size_add) {
    string s;
    OE_ASSERT_OK(s.append("xx", 2));
    auto r = s.append(reinterpret_cast<const char*>(1),
                      std::numeric_limits<std::size_t>::max());
    OE_ASSERT(!r.ok());
    OE_ASSERT(r.code() == error_code::overflow ||
              r.code() == error_code::capacity_exceeded);
}

OE_TEST(string_assign_replaces) {
    auto r = string::from_cstr("old");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.assign(std::string_view("new value")));
    OE_ASSERT_EQ(s.size(), std::size_t{9});
    OE_ASSERT(std::strcmp(s.c_str(), "new value") == 0);
}

OE_TEST(string_assign_empty_clears) {
    auto r = string::from_cstr("data");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.assign(std::string_view("")));
    OE_ASSERT(s.empty());
    OE_ASSERT(std::strcmp(s.c_str(), "") == 0);
}

OE_TEST(string_insert_middle) {
    auto r = string::from_cstr("axxxc");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.insert(2, std::string_view("BBB")));
    OE_ASSERT(std::strcmp(s.c_str(), "axBBBxxc") == 0);
}

OE_TEST(string_insert_front) {
    auto r = string::from_cstr("world");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.insert(0, std::string_view("hello ")));
    OE_ASSERT(std::strcmp(s.c_str(), "hello world") == 0);
}

OE_TEST(string_insert_end) {
    auto r = string::from_cstr("hi");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.insert(s.size(), std::string_view(" there")));
    OE_ASSERT(std::strcmp(s.c_str(), "hi there") == 0);
}

OE_TEST(string_insert_out_of_range) {
    auto r = string::from_cstr("ab");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    auto x = s.insert(3, std::string_view("X"));
    OE_ASSERT(!x.ok());
    OE_ASSERT_EQ(x.code(), error_code::out_of_range);
}

OE_TEST(string_replace_shrink) {
    auto r = string::from_cstr("abcdefgh");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.replace(2, 4, std::string_view("Z")));
    OE_ASSERT(std::strcmp(s.c_str(), "abZgh") == 0);
}

OE_TEST(string_replace_grow) {
    auto r = string::from_cstr("abcdefgh");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.replace(2, 2, std::string_view("XXXXX")));
    OE_ASSERT(std::strcmp(s.c_str(), "abXXXXXefgh") == 0);
}

OE_TEST(string_replace_same_size) {
    auto r = string::from_cstr("abcdef");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.replace(1, 3, std::string_view("XYZ")));
    OE_ASSERT(std::strcmp(s.c_str(), "aXYZef") == 0);
}

OE_TEST(string_replace_out_of_range) {
    auto r = string::from_cstr("abc");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    auto x = s.replace(2, 5, std::string_view("X"));
    OE_ASSERT(!x.ok());
    OE_ASSERT_EQ(x.code(), error_code::out_of_range);
}

OE_TEST(string_replace_overflow) {
    auto r = string::from_cstr("abc");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    auto x = s.replace(std::numeric_limits<std::size_t>::max(),
                       1, std::string_view("X"));
    OE_ASSERT(!x.ok());
    OE_ASSERT_EQ(x.code(), error_code::overflow);
}

OE_TEST(string_substr_basic) {
    auto r = string::from_cstr("abcdef");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    auto sub = s.substr(2, 3);
    OE_ASSERT_OK(sub);
    OE_ASSERT(std::strcmp(sub.value().c_str(), "cde") == 0);
}

OE_TEST(string_substr_zero_len) {
    auto r = string::from_cstr("abc");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    auto sub = s.substr(1, 0);
    OE_ASSERT_OK(sub);
    OE_ASSERT(sub.value().empty());
}

OE_TEST(string_substr_at_end) {
    auto r = string::from_cstr("abc");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    auto sub = s.substr(3, 0);
    OE_ASSERT_OK(sub);
    OE_ASSERT(sub.value().empty());
}

OE_TEST(string_substr_out_of_range) {
    auto r = string::from_cstr("abc");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    auto sub = s.substr(2, 5);
    OE_ASSERT(!sub.ok());
    OE_ASSERT_EQ(sub.code(), error_code::out_of_range);
}

OE_TEST(string_at_bounds) {
    auto r = string::from_cstr("ab");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_EQ(s.at(0).value(), 'a');
    OE_ASSERT_EQ(s.at(1).value(), 'b');
    auto x = s.at(2);
    OE_ASSERT(!x.ok());
    OE_ASSERT_EQ(x.code(), error_code::out_of_range);
}

OE_TEST(string_clear_keeps_capacity) {
    auto r = string::from_cstr("hello");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    std::size_t cap = s.capacity();
    s.clear();
    OE_ASSERT(s.empty());
    OE_ASSERT_EQ(s.capacity(), cap);
    OE_ASSERT(std::strcmp(s.c_str(), "") == 0);
}

OE_TEST(string_copy_to_cstr_exact_fit) {
    auto r = string::from_cstr("abc");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    char out[4] = {};
    auto cp = s.copy_to_cstr(out, 4);
    OE_ASSERT_OK(cp);
    OE_ASSERT_EQ(cp.value(), std::size_t{3});
    OE_ASSERT(std::strcmp(out, "abc") == 0);
}

OE_TEST(string_copy_to_cstr_truncates) {
    auto r = string::from_cstr("longer");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    char out[4] = {};
    auto cp = s.copy_to_cstr(out, 4);
    OE_ASSERT(!cp.ok());
    OE_ASSERT_EQ(cp.code(), error_code::truncated);
    OE_ASSERT(std::strcmp(out, "lon") == 0);  // 3 chars + null
}

OE_TEST(string_copy_to_cstr_zero_len) {
    auto r = string::from_cstr("x");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    char out[1] = {};
    auto cp = s.copy_to_cstr(out, 0);
    OE_ASSERT(!cp.ok());
    OE_ASSERT_EQ(cp.code(), error_code::invalid_argument);
}

OE_TEST(string_copy_to_cstr_one_byte_buffer) {
    auto r = string::from_cstr("x");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    char out[1] = {1};
    auto cp = s.copy_to_cstr(out, 1);
    OE_ASSERT(!cp.ok());
    OE_ASSERT_EQ(cp.code(), error_code::truncated);
    OE_ASSERT_EQ(out[0], char{0});
}

OE_TEST(string_clone_independent) {
    auto r = string::from_cstr("abc");
    OE_ASSERT_OK(r);
    auto a = std::move(r).value();
    auto cl = a.clone();
    OE_ASSERT_OK(cl);
    auto b = std::move(cl).value();
    OE_ASSERT_OK(a.append('Z'));
    OE_ASSERT(std::strcmp(a.c_str(), "abcZ") == 0);
    OE_ASSERT(std::strcmp(b.c_str(), "abc") == 0);
}

OE_TEST(string_move_clears_source) {
    auto r = string::from_cstr("data");
    OE_ASSERT_OK(r);
    auto a = std::move(r).value();
    auto b = std::move(a);
    OE_ASSERT_EQ(a.size(), std::size_t{0});
    OE_ASSERT(std::strcmp(a.c_str(), "") == 0);
    OE_ASSERT(std::strcmp(b.c_str(), "data") == 0);
}

OE_TEST(string_reserve_grows) {
    string s;
    OE_ASSERT_OK(s.reserve(100));
    OE_ASSERT(s.capacity() >= 100);
    OE_ASSERT_EQ(s.size(), std::size_t{0});
    OE_ASSERT(std::strcmp(s.c_str(), "") == 0);
}

OE_TEST(string_append_to_near_full) {
    string s;
    OE_ASSERT_OK(s.reserve(8));
    for (int i = 0; i < 8; ++i) OE_ASSERT_OK(s.append(static_cast<char>('a' + i)));
    OE_ASSERT_EQ(s.size(), std::size_t{8});
    OE_ASSERT_OK(s.append('I'));
    OE_ASSERT_EQ(s.size(), std::size_t{9});
    OE_ASSERT(std::strcmp(s.c_str(), "abcdefghI") == 0);
}

OE_TEST(string_self_append_grows_safely) {
    auto r = string::from_cstr("ABCD");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.append(s.view()));
    OE_ASSERT(std::strcmp(s.c_str(), "ABCDABCD") == 0);
    OE_ASSERT_EQ(s.size(), std::size_t{8});
}

OE_TEST(string_self_append_repeated) {
    auto r = string::from_cstr("x");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    for (int i = 0; i < 5; ++i) OE_ASSERT_OK(s.append(s.view()));
    // 1 -> 2 -> 4 -> 8 -> 16 -> 32
    OE_ASSERT_EQ(s.size(), std::size_t{32});
    for (std::size_t i = 0; i < s.size(); ++i) {
        OE_ASSERT_EQ(s.at(i).value(), 'x');
    }
}

OE_TEST(string_self_insert_at_zero_duplicates) {
    auto r = string::from_cstr("XY");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.insert(0, s.view()));
    OE_ASSERT(std::strcmp(s.c_str(), "XYXY") == 0);
}

OE_TEST(string_self_insert_at_end_appends) {
    auto r = string::from_cstr("AB");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.insert(s.size(), s.view()));
    OE_ASSERT(std::strcmp(s.c_str(), "ABAB") == 0);
}

OE_TEST(string_self_assign_idempotent) {
    auto r = string::from_cstr("hello");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.assign(s.view()));
    OE_ASSERT(std::strcmp(s.c_str(), "hello") == 0);
    OE_ASSERT_EQ(s.size(), std::size_t{5});
}

OE_TEST(string_self_replace_with_prefix) {
    auto r = string::from_cstr("abcdef");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    auto v = s.view().substr(0, 2);  // "ab", inside s
    OE_ASSERT_OK(s.replace(4, 2, v));
    OE_ASSERT(std::strcmp(s.c_str(), "abcdab") == 0);
}

OE_TEST(string_self_replace_with_tail) {
    auto r = string::from_cstr("abcdef");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    auto v = s.view().substr(4, 2);  // "ef", inside s
    OE_ASSERT_OK(s.replace(0, 2, v));
    OE_ASSERT(std::strcmp(s.c_str(), "efcdef") == 0);
}

OE_TEST(string_secure_clear_zeroes) {
    auto r = string::from_cstr("TOPSECRET");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    const char* snap = s.data();
    std::size_t old_size = s.size();
    s.secure_clear();
    OE_ASSERT(s.empty());
    OE_ASSERT(std::strcmp(s.c_str(), "") == 0);
    for (std::size_t i = 0; i < old_size; ++i) {
        OE_ASSERT_EQ(snap[i], char{0});
    }
}

OE_TEST(string_verify_default) {
    string s;
    OE_ASSERT(s.verify());
}

OE_TEST(string_verify_through_lifecycle) {
    auto r = string::from_cstr("abc");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT(s.verify());
    OE_ASSERT_OK(s.append("DEF", 3));
    OE_ASSERT(s.verify());
    OE_ASSERT_OK(s.insert(0, std::string_view("X")));
    OE_ASSERT(s.verify());
    OE_ASSERT_OK(s.replace(0, 2, std::string_view("YYY")));
    OE_ASSERT(s.verify());
    OE_ASSERT_OK(s.assign(std::string_view("done")));
    OE_ASSERT(s.verify());
    s.clear();
    OE_ASSERT(s.verify());
    s.secure_clear();
    OE_ASSERT(s.verify());
}

OE_TEST(string_self_move_assign_safe) {
    auto r = string::from_cstr("payload");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    s = std::move(s);
    OE_ASSERT_EQ(s.size(), std::size_t{7});
    OE_ASSERT(std::strcmp(s.c_str(), "payload") == 0);
    OE_ASSERT(s.verify());
}

OE_TEST(string_replace_whole_with_longer) {
    auto r = string::from_cstr("ab");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.replace(0, 2, std::string_view("XYZW")));
    OE_ASSERT(std::strcmp(s.c_str(), "XYZW") == 0);
    OE_ASSERT_EQ(s.size(), std::size_t{4});
}

OE_TEST(string_replace_whole_with_shorter) {
    auto r = string::from_cstr("abcdef");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.replace(0, 6, std::string_view("X")));
    OE_ASSERT(std::strcmp(s.c_str(), "X") == 0);
    OE_ASSERT_EQ(s.size(), std::size_t{1});
}

OE_TEST(string_replace_middle_with_empty_deletes_range) {
    auto r = string::from_cstr("abcdef");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.replace(1, 3, std::string_view("")));
    OE_ASSERT(std::strcmp(s.c_str(), "aef") == 0);
}

OE_TEST(string_replace_at_end_zero_length_inserts) {
    auto r = string::from_cstr("ab");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.replace(s.size(), 0, std::string_view("CD")));
    OE_ASSERT(std::strcmp(s.c_str(), "abCD") == 0);
}

OE_TEST(string_insert_empty_view_is_noop) {
    auto r = string::from_cstr("abc");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_OK(s.insert(1, std::string_view{}));
    OE_ASSERT(std::strcmp(s.c_str(), "abc") == 0);
}

OE_TEST(string_secure_clear_idempotent) {
    auto r = string::from_cstr("data");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    s.secure_clear();
    s.secure_clear();
    s.secure_clear();
    OE_ASSERT(s.empty());
    OE_ASSERT(s.verify());
    OE_ASSERT(std::strcmp(s.c_str(), "") == 0);
}

OE_TEST(string_append_after_secure_clear_reusable) {
    auto r = string::from_cstr("OLD");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    s.secure_clear();
    OE_ASSERT_OK(s.append(std::string_view("NEW")));
    OE_ASSERT(std::strcmp(s.c_str(), "NEW") == 0);
    OE_ASSERT_EQ(s.size(), std::size_t{3});
    OE_ASSERT(s.verify());
}

OE_TEST(string_all_nul_contents_preserved) {
    const char data[5] = {0, 0, 0, 0, 0};
    auto r = string::from_bytes(data, 5);
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    OE_ASSERT_EQ(s.size(), std::size_t{5});
    OE_ASSERT_EQ(s.view().size(), std::size_t{5});
    // c_str sees the first embedded NUL and stops there. Expected.
    OE_ASSERT(std::strcmp(s.c_str(), "") == 0);

    auto cl = s.clone();
    OE_ASSERT_OK(cl);
    auto c = std::move(cl).value();
    OE_ASSERT_EQ(c.size(), std::size_t{5});
    for (std::size_t i = 0; i < 5; ++i) {
        OE_ASSERT_EQ(c.at(i).value(), char{0});
    }
}

OE_TEST(string_binary_through_substr_and_clone) {
    const char data[] = {'a', 0, 'b', 0, 'c'};
    auto r = string::from_bytes(data, 5);
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    auto sub = s.substr(1, 3);
    OE_ASSERT_OK(sub);
    auto t = std::move(sub).value();
    OE_ASSERT_EQ(t.size(), std::size_t{3});
    OE_ASSERT_EQ(t.at(0).value(), char{0});
    OE_ASSERT_EQ(t.at(1).value(), char{'b'});
    OE_ASSERT_EQ(t.at(2).value(), char{0});
}

OE_TEST(string_copy_to_cstr_exactly_fits) {
    auto r = string::from_cstr("abc");
    OE_ASSERT_OK(r);
    auto s = std::move(r).value();
    char out[4] = {};
    auto cp = s.copy_to_cstr(out, 4);  // 3 chars + terminator
    OE_ASSERT_OK(cp);
    OE_ASSERT_EQ(cp.value(), std::size_t{3});
    OE_ASSERT(std::strcmp(out, "abc") == 0);
    OE_ASSERT_EQ(out[3], char{0});
}

OE_TEST(string_replace_in_chain_with_realloc) {
    string s;
    OE_ASSERT_OK(s.append('a'));
    for (int i = 0; i < 6; ++i) {
        // Each replace doubles a part of the string, forcing several
        // realloc cycles. Validates that invariants hold across them.
        OE_ASSERT_OK(s.replace(0, s.size(), s.view()));
        OE_ASSERT_OK(s.append(s.view()));
    }
    OE_ASSERT(s.verify());
    for (std::size_t i = 0; i < s.size(); ++i) {
        OE_ASSERT_EQ(s.at(i).value(), 'a');
    }
}
