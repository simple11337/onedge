// Tiny header-only test harness. Zero dependencies. Each translation unit
// registers tests via OE_TEST(name) and asserts via OE_ASSERT(cond).
// test_main.cpp provides the entry point.

#ifndef ONEDGE_TEST_HARNESS_H
#define ONEDGE_TEST_HARNESS_H

#include <cstdio>
#include <cstring>
#include <vector>

namespace oe_test {

struct test_case {
    const char* name;
    void (*fn)();
};

inline std::vector<test_case>& registry() {
    static std::vector<test_case> v;
    return v;
}

inline int& current_failures() {
    static int n = 0;
    return n;
}

struct registrar {
    registrar(const char* name, void (*fn)()) {
        registry().push_back({name, fn});
    }
};

}  // namespace oe_test

#define OE_CONCAT_INNER(a, b) a##b
#define OE_CONCAT(a, b) OE_CONCAT_INNER(a, b)

#define OE_TEST(name)                                                                                  \
    static void OE_CONCAT(oe_test_fn_, name)();                                                        \
    static ::oe_test::registrar OE_CONCAT(oe_test_reg_, name){#name, &OE_CONCAT(oe_test_fn_, name)};   \
    static void OE_CONCAT(oe_test_fn_, name)()

#define OE_ASSERT(cond)                                                                                \
    do {                                                                                               \
        if (!(cond)) {                                                                                 \
            std::fprintf(stderr, "  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);                     \
            ::oe_test::current_failures()++;                                                           \
        }                                                                                              \
    } while (0)

#define OE_ASSERT_EQ(a, b)                                                                             \
    do {                                                                                               \
        auto _aa = (a);                                                                                \
        auto _bb = (b);                                                                                \
        if (!(_aa == _bb)) {                                                                           \
            std::fprintf(stderr, "  FAIL %s:%d  %s == %s\n", __FILE__, __LINE__, #a, #b);              \
            ::oe_test::current_failures()++;                                                           \
        }                                                                                              \
    } while (0)

// Bind to a forwarding reference so result<T> for move-only T (buffer,
// string) is not copied when checked against an lvalue.
#define OE_ASSERT_OK(expr)                                                                             \
    do {                                                                                               \
        auto&& _r = (expr);                                                                            \
        if (!_r.ok()) {                                                                                \
            std::fprintf(stderr, "  FAIL %s:%d  %s: %s\n", __FILE__, __LINE__, #expr, _r.message());   \
            ::oe_test::current_failures()++;                                                           \
        }                                                                                              \
    } while (0)

#define OE_ASSERT_ERR(expr, expected_code)                                                             \
    do {                                                                                               \
        auto&& _r = (expr);                                                                            \
        if (_r.ok()) {                                                                                 \
            std::fprintf(stderr, "  FAIL %s:%d  %s expected error %s, got ok\n",                       \
                         __FILE__, __LINE__, #expr, #expected_code);                                   \
            ::oe_test::current_failures()++;                                                           \
        } else if (_r.code() != (expected_code)) {                                                     \
            std::fprintf(stderr, "  FAIL %s:%d  %s expected %s, got %s\n", __FILE__, __LINE__, #expr,  \
                         #expected_code, _r.message());                                                \
            ::oe_test::current_failures()++;                                                           \
        }                                                                                              \
    } while (0)

#endif  // ONEDGE_TEST_HARNESS_H
