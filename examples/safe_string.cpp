// Safe string example. Builds, mutates, slices, and copies out to a
// fixed-size C buffer with explicit truncation detection.

#include "onedge/string.h"

#include <cstdio>

int main() {
    auto r = oe::string::from_cstr("hello");
    if (!r.ok()) {
        std::fprintf(stderr, "construction failed: %s\n", r.message());
        return 1;
    }
    auto s = std::move(r).value();

    if (auto a = s.append(std::string_view(", world")); !a.ok()) {
        std::fprintf(stderr, "append failed: %s\n", a.message());
        return 1;
    }

    std::printf("string: '%s' (size=%zu)\n", s.c_str(), s.size());

    auto sub = s.substr(7, 5);  // "world"
    if (!sub.ok()) {
        std::fprintf(stderr, "substr failed: %s\n", sub.message());
        return 1;
    }
    std::printf("substr: '%s'\n", sub.value().c_str());

    char out[6] = {};
    auto cp = s.copy_to_cstr(out, sizeof(out));
    if (cp.code() == oe::error_code::truncated) {
        std::printf("copy was truncated, got: '%s'\n", out);
    } else if (cp.ok()) {
        std::printf("copied %zu chars: '%s'\n", cp.value(), out);
    } else {
        std::fprintf(stderr, "copy failed: %s\n", cp.message());
        return 1;
    }

    return 0;
}
