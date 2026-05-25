#ifndef ONEDGE_FIXED_BUFFER_H
#define ONEDGE_FIXED_BUFFER_H

#include "onedge/onedge_config.h"
#include "onedge/checked_math.h"
#include "onedge/result.h"
#include "onedge/span.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace oe {

// Stack-storage byte buffer. Same checks as oe::buffer, no heap, fixed N.
// Destructor does NOT zero; call secure_clear() if needed.
template <std::size_t N>
class fixed_buffer {
    static_assert(N > 0, "fixed_buffer requires N > 0");

public:
    constexpr fixed_buffer() noexcept = default;

    constexpr std::size_t capacity() const noexcept { return N; }
    std::size_t           size() const noexcept     { return size_; }
    bool                  empty() const noexcept    { return size_ == 0; }

    const std::uint8_t* data() const noexcept { return storage_; }
    std::uint8_t*       data() noexcept       { return storage_; }

    byte_span       as_span() noexcept       { return byte_span(storage_, size_); }
    const_byte_span as_span() const noexcept { return const_byte_span(storage_, size_); }

    ONEDGE_NODISCARD result<std::uint8_t> at(std::size_t i) const noexcept {
        if (i >= size_) return result<std::uint8_t>::failure(error_code::out_of_range);
        return result<std::uint8_t>::success(storage_[i]);
    }

    ONEDGE_NODISCARD result<void> set(std::size_t i, std::uint8_t v) noexcept {
        if (i >= size_) return result<void>::failure(error_code::out_of_range);
        storage_[i] = v;
        return result<void>::success();
    }

    ONEDGE_NODISCARD result<void> append(std::uint8_t b) noexcept {
        if (size_ >= N) return result<void>::failure(error_code::capacity_exceeded);
        storage_[size_++] = b;
        return result<void>::success();
    }

    ONEDGE_NODISCARD result<void> append(const void* data, std::size_t len) noexcept {
        if (len == 0) return result<void>::success();
        if (!data) return result<void>::failure(error_code::null_pointer);
        auto new_size_opt = checked_add(size_, len);
        if (!new_size_opt) return result<void>::failure(error_code::overflow);
        if (*new_size_opt > N) return result<void>::failure(error_code::capacity_exceeded);
        std::memmove(storage_ + size_, data, len);
        size_ = *new_size_opt;
        return result<void>::success();
    }

    ONEDGE_NODISCARD result<void> resize(std::size_t new_size, std::uint8_t fill_byte = 0) noexcept {
        if (new_size > N) return result<void>::failure(error_code::capacity_exceeded);
        if (new_size > size_) std::memset(storage_ + size_, fill_byte, new_size - size_);
        size_ = new_size;
        return result<void>::success();
    }

    void clear() noexcept { size_ = 0; }

    // Zeros all N bytes (not just used portion) and resets size.
    void secure_clear() noexcept {
        detail::secure_zero(storage_, N);
        size_ = 0;
    }

    void fill(std::uint8_t byte) noexcept {
        if (size_ > 0) std::memset(storage_, byte, size_);
    }

    ONEDGE_NODISCARD bool verify() const noexcept { return size_ <= N; }

private:
    std::uint8_t storage_[N] = {};
    std::size_t  size_       = 0;
};

}  // namespace oe

#endif  // ONEDGE_FIXED_BUFFER_H
