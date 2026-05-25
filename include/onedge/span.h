#ifndef ONEDGE_SPAN_H
#define ONEDGE_SPAN_H

#include "onedge/onedge_config.h"
#include "onedge/result.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace oe {

// Non-owning view over a contiguous range. std::span is C++20, this
// works on C++17 and slices through result<>.
template <typename T>
class span {
public:
    using element_type    = T;
    using value_type      = typename std::remove_cv<T>::type;
    using size_type       = std::size_t;
    using pointer         = T*;
    using reference       = T&;
    using iterator        = T*;
    using const_iterator  = const T*;

    constexpr span() noexcept = default;
    constexpr span(T* data, size_type size) noexcept : data_(data), size_(size) {}

    template <std::size_t N>
    constexpr span(T (&arr)[N]) noexcept : data_(arr), size_(N) {}

    // span<T> -> span<const T>.
    template <typename U,
              typename = typename std::enable_if<
                  std::is_same<U, typename std::remove_const<T>::type>::value>::type>
    constexpr span(span<U> other) noexcept : data_(other.data()), size_(other.size()) {}

    constexpr T*        data() const noexcept       { return data_; }
    constexpr size_type size() const noexcept       { return size_; }
    constexpr size_type size_bytes() const noexcept { return size_ * sizeof(T); }
    constexpr bool      empty() const noexcept      { return size_ == 0; }

    constexpr iterator begin() const noexcept { return data_; }
    constexpr iterator end() const noexcept   { return data_ + size_; }

    // Unchecked. Caller must know i < size().
    constexpr T& operator[](size_type i) const noexcept { return data_[i]; }

    constexpr bool in_bounds(size_type i) const noexcept { return i < size_; }

    // Returns &element or null if OOB.
    ONEDGE_NODISCARD T* at(size_type i) const noexcept {
        return i < size_ ? data_ + i : nullptr;
    }

    ONEDGE_NODISCARD result<span> subspan(size_type offset, size_type count) const noexcept {
        if (offset > size_) return result<span>::failure(error_code::out_of_range);
        if (count > size_ - offset) return result<span>::failure(error_code::out_of_range);
        return result<span>::success(span(data_ + offset, count));
    }

    // Caps silently when count > size(). subspan() if you want to detect that.
    constexpr span first(size_type count) const noexcept {
        size_type take = count < size_ ? count : size_;
        return span(data_, take);
    }
    constexpr span last(size_type count) const noexcept {
        size_type take = count < size_ ? count : size_;
        return span(data_ + (size_ - take), take);
    }

private:
    T*        data_ = nullptr;
    size_type size_ = 0;
};

using byte_span       = span<std::uint8_t>;
using const_byte_span = span<const std::uint8_t>;

template <typename T>
constexpr const_byte_span as_bytes(span<T> s) noexcept {
    return const_byte_span(
        reinterpret_cast<const std::uint8_t*>(s.data()), s.size_bytes());
}

}  // namespace oe

#endif  // ONEDGE_SPAN_H
