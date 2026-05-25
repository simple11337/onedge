// Safe buffer example. Demonstrates explicit bounds checks and how the
// library refuses overflow rather than silently truncating.

#include "onedge/buffer.h"

#include <cstdio>
#include <cstdint>
#include <limits>

int main() {
    auto r = oe::buffer::with_capacity(16);
    if (!r.ok()) {
        std::fprintf(stderr, "alloc failed: %s\n", r.message());
        return 1;
    }
    auto b = std::move(r).value();

    if (auto a = b.resize(8, 0); !a.ok()) {
        std::fprintf(stderr, "resize failed: %s\n", a.message());
        return 1;
    }

    const std::uint8_t payload[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    if (auto w = b.write(2, payload, sizeof(payload)); !w.ok()) {
        std::fprintf(stderr, "write failed: %s\n", w.message());
        return 1;
    }

    std::printf("buffer (%zu bytes): ", b.size());
    for (std::size_t i = 0; i < b.size(); ++i) {
        std::printf("%02X ", b.at(i).value());
    }
    std::printf("\n");

    // This will be refused with out_of_range instead of corrupting memory.
    auto bad = b.write(7, payload, sizeof(payload));
    if (!bad.ok()) {
        std::printf("rejected overlong write: %s\n", bad.message());
    }

    // Overflow case: adding SIZE_MAX bytes is refused without allocating.
    auto huge = b.append(reinterpret_cast<const void*>(1),
                         std::numeric_limits<std::size_t>::max());
    if (!huge.ok()) {
        std::printf("rejected oversized append: %s\n", huge.message());
    }

    return 0;
}
