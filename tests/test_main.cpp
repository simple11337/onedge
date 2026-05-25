#include "test_harness.h"

#include <cstdio>

int main() {
    auto& tests = oe_test::registry();
    int passed = 0;
    int failed = 0;
    for (auto& t : tests) {
        int before = oe_test::current_failures();
        std::printf("[ RUN  ] %s\n", t.name);
        t.fn();
        int delta = oe_test::current_failures() - before;
        if (delta == 0) {
            ++passed;
            std::printf("[  OK  ] %s\n", t.name);
        } else {
            ++failed;
            std::printf("[ FAIL ] %s  (%d assertions failed)\n", t.name, delta);
        }
    }
    std::printf("\nSummary: %d passed, %d failed, %zu total\n", passed, failed, tests.size());
    return failed == 0 ? 0 : 1;
}
