#ifndef ONEDGE_CHECKED_MATH_H
#define ONEDGE_CHECKED_MATH_H

#include "onedge/onedge_config.h"
#include "onedge/result.h"

#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>

namespace oe {

// Size_t wraparound is a big chunk of heap corruption. Use these any
// time the inputs aren't provably bounded.

ONEDGE_NODISCARD inline std::optional<std::size_t> checked_add(std::size_t a, std::size_t b) noexcept {
    if (a > std::numeric_limits<std::size_t>::max() - b) return std::nullopt;
    return a + b;
}

ONEDGE_NODISCARD inline std::optional<std::size_t> checked_sub(std::size_t a, std::size_t b) noexcept {
    if (b > a) return std::nullopt;
    return a - b;
}

ONEDGE_NODISCARD inline std::optional<std::size_t> checked_mul(std::size_t a, std::size_t b) noexcept {
    if (a == 0 || b == 0) return std::size_t{0};
    if (a > std::numeric_limits<std::size_t>::max() / b) return std::nullopt;
    return a * b;
}

ONEDGE_NODISCARD inline result<std::size_t> safe_add(std::size_t a, std::size_t b) {
    auto v = checked_add(a, b);
    if (!v) return result<std::size_t>::failure(error_code::overflow);
    return result<std::size_t>::success(*v);
}
ONEDGE_NODISCARD inline result<std::size_t> safe_sub(std::size_t a, std::size_t b) {
    auto v = checked_sub(a, b);
    if (!v) return result<std::size_t>::failure(error_code::overflow);
    return result<std::size_t>::success(*v);
}
ONEDGE_NODISCARD inline result<std::size_t> safe_mul(std::size_t a, std::size_t b) {
    auto v = checked_mul(a, b);
    if (!v) return result<std::size_t>::failure(error_code::overflow);
    return result<std::size_t>::success(*v);
}

// current + additional, also capped against the alloc cap.
ONEDGE_NODISCARD inline std::optional<std::size_t> checked_grow(std::size_t current,
                                                                std::size_t additional,
                                                                std::size_t cap = ONEDGE_MAX_ALLOC_BYTES) noexcept {
    auto sum = checked_add(current, additional);
    if (!sum) return std::nullopt;
    if (*sum > cap) return std::nullopt;
    return *sum;
}

// True iff offset + length stays inside [0, container_size].
ONEDGE_NODISCARD inline bool range_in_bounds(std::size_t offset,
                                             std::size_t length,
                                             std::size_t container_size) noexcept {
    auto end = checked_add(offset, length);
    if (!end) return false;
    return *end <= container_size;
}

// Narrowing-aware int cast. nullopt on out-of-range.
template <typename Dst, typename Src>
ONEDGE_NODISCARD inline std::optional<Dst> checked_cast(Src v) noexcept {
    static_assert(std::is_integral<Src>::value, "checked_cast: integral source required");
    static_assert(std::is_integral<Dst>::value, "checked_cast: integral destination required");
    using SLim = std::numeric_limits<Src>;
    using DLim = std::numeric_limits<Dst>;

    if constexpr (SLim::is_signed == DLim::is_signed) {
        if (v < static_cast<Src>(DLim::min())) return std::nullopt;
        if (v > static_cast<Src>(DLim::max())) return std::nullopt;
    } else if constexpr (SLim::is_signed && !DLim::is_signed) {
        if (v < 0) return std::nullopt;
        using U = std::make_unsigned_t<Src>;
        if (static_cast<U>(v) > static_cast<U>(DLim::max())) return std::nullopt;
    } else {
        if (v > static_cast<Src>(DLim::max())) return std::nullopt;
    }
    return static_cast<Dst>(v);
}

}  // namespace oe

#endif  // ONEDGE_CHECKED_MATH_H
