#include "onedge/buffer.h"
#include "onedge/checked_math.h"

#include <cstdlib>
#include <cstring>

namespace oe {

namespace {

constexpr std::size_t kMinGrowth = 16;

// Geometric growth, capped against overflow and the alloc cap.
std::size_t next_capacity(std::size_t current, std::size_t needed) noexcept {
    std::size_t grown = current;
    if (grown < kMinGrowth) grown = kMinGrowth;
    auto doubled = checked_mul(grown, 2);
    if (doubled && *doubled >= needed && *doubled <= ONEDGE_MAX_ALLOC_BYTES) {
        return *doubled;
    }
    return needed;
}

}  // namespace

bool buffer::verify() const noexcept {
    if (cap_ == 0) return data_ == nullptr && size_ == 0;
    if (data_ == nullptr) return false;
    if (size_ > cap_) return false;
    return true;
}

#define BUFFER_AUDIT()                              \
    do {                                            \
        ONEDGE_CHECK(size_ <= cap_);                \
        ONEDGE_CHECK(cap_ == 0 || data_ != nullptr); \
        ONEDGE_CHECK(cap_ != 0 || data_ == nullptr); \
    } while (0)

buffer::~buffer() noexcept {
    // moved-from buffer is legitimately empty, so no audit here.
    std::free(data_);
}

buffer::buffer(buffer&& other) noexcept
    : data_(other.data_), size_(other.size_), cap_(other.cap_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.cap_  = 0;
}

buffer& buffer::operator=(buffer&& other) noexcept {
    if (this != &other) {
        std::free(data_);
        data_ = other.data_;
        size_ = other.size_;
        cap_  = other.cap_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.cap_  = 0;
    }
    return *this;
}

result<buffer> buffer::with_capacity(std::size_t initial_capacity) {
    buffer b;
    if (initial_capacity == 0) return result<buffer>::success(std::move(b));
    if (initial_capacity > ONEDGE_MAX_ALLOC_BYTES) {
        return result<buffer>::failure(error_code::capacity_exceeded);
    }
    void* p = std::malloc(initial_capacity);
    if (!p) return result<buffer>::failure(error_code::out_of_memory);
    b.data_ = static_cast<std::uint8_t*>(p);
    b.cap_  = initial_capacity;
    b.size_ = 0;
    return result<buffer>::success(std::move(b));
}

result<buffer> buffer::from_bytes(const void* data, std::size_t len) {
    if (len == 0) return with_capacity(0);
    if (!data) return result<buffer>::failure(error_code::null_pointer);
    auto r = with_capacity(len);
    if (!r.ok()) return r;
    buffer b = std::move(r).value();
    std::memcpy(b.data_, data, len);  // fresh alloc, no aliasing
    b.size_ = len;
    return result<buffer>::success(std::move(b));
}

result<buffer> buffer::clone() const {
    BUFFER_AUDIT();
    return from_bytes(data_, size_);
}

result<std::uint8_t> buffer::at(std::size_t i) const noexcept {
    BUFFER_AUDIT();
    if (i >= size_) return result<std::uint8_t>::failure(error_code::out_of_range);
    return result<std::uint8_t>::success(data_[i]);
}

result<void> buffer::set(std::size_t i, std::uint8_t value) noexcept {
    BUFFER_AUDIT();
    if (i >= size_) return result<void>::failure(error_code::out_of_range);
    data_[i] = value;
    return result<void>::success();
}

result<void> buffer::reserve(std::size_t new_cap) {
    BUFFER_AUDIT();
    if (new_cap <= cap_) return result<void>::success();
    if (new_cap > ONEDGE_MAX_ALLOC_BYTES) {
        return result<void>::failure(error_code::capacity_exceeded);
    }
    void* p = std::realloc(data_, new_cap);
    if (!p) return result<void>::failure(error_code::out_of_memory);
    data_ = static_cast<std::uint8_t*>(p);
    cap_  = new_cap;
    return result<void>::success();
}

result<void> buffer::grow_to_at_least(std::size_t needed) {
    if (needed <= cap_) return result<void>::success();
    std::size_t target = next_capacity(cap_, needed);
    if (target < needed) target = needed;
    return reserve(target);
}

result<void> buffer::resize(std::size_t new_size, std::uint8_t fill_byte) {
    BUFFER_AUDIT();
    if (new_size > ONEDGE_MAX_ALLOC_BYTES) {
        return result<void>::failure(error_code::capacity_exceeded);
    }
    if (new_size > cap_) {
        auto r = grow_to_at_least(new_size);
        if (!r.ok()) return r;
    }
    if (new_size > size_) {
        std::memset(data_ + size_, fill_byte, new_size - size_);
    }
    // note: we never shrink the allocation here. would be nice but
    // realloc to smaller is platform-iffy and most callers don't care.
    size_ = new_size;
    return result<void>::success();
}

void buffer::clear() noexcept {
    BUFFER_AUDIT();
    size_ = 0;
}

void buffer::secure_clear() noexcept {
    BUFFER_AUDIT();
    detail::secure_zero(data_, size_);
    size_ = 0;
}

result<void> buffer::copy_in_possibly_aliased(std::size_t dst_offset,
                                              const void* src,
                                              std::size_t len) {
    // Snapshot aliasing before grow can invalidate src.
    bool        aliased    = detail::pointer_in_range(src, data_, cap_);
    std::size_t src_offset = 0;
    if (aliased) {
        src_offset = static_cast<std::size_t>(
            static_cast<const std::uint8_t*>(src) - data_);
    }

    auto end_opt = checked_add(dst_offset, len);
    if (!end_opt) return result<void>::failure(error_code::overflow);
    if (*end_opt > cap_) {
        auto r = grow_to_at_least(*end_opt);
        if (!r.ok()) return r;
    }

    // Recompute src if it lived inside us; memmove handles overlap.
    const void* effective_src = aliased ? static_cast<const void*>(data_ + src_offset) : src;
    std::memmove(data_ + dst_offset, effective_src, len);
    return result<void>::success();
}

result<void> buffer::append(std::uint8_t byte) {
    BUFFER_AUDIT();
    auto new_size_opt = checked_add(size_, 1);
    if (!new_size_opt) return result<void>::failure(error_code::overflow);
    auto r = grow_to_at_least(*new_size_opt);
    if (!r.ok()) return r;
    data_[size_] = byte;
    size_ = *new_size_opt;
    return result<void>::success();
}

result<void> buffer::append(const void* data, std::size_t len) {
    BUFFER_AUDIT();
    if (len == 0) return result<void>::success();
    if (!data) return result<void>::failure(error_code::null_pointer);
    auto new_size_opt = checked_add(size_, len);
    if (!new_size_opt) return result<void>::failure(error_code::overflow);
    auto r = copy_in_possibly_aliased(size_, data, len);
    if (!r.ok()) return r;
    size_ = *new_size_opt;
    return result<void>::success();
}

result<void> buffer::write(std::size_t offset, const void* src, std::size_t len) {
    BUFFER_AUDIT();
    if (len == 0) return result<void>::success();
    if (!src) return result<void>::failure(error_code::null_pointer);
    if (!range_in_bounds(offset, len, size_)) {
        auto end = checked_add(offset, len);
        if (!end) return result<void>::failure(error_code::overflow);
        return result<void>::failure(error_code::out_of_range);
    }
    // No grow here; memmove for overlap.
    std::memmove(data_ + offset, src, len);
    return result<void>::success();
}

result<std::size_t> buffer::read(std::size_t offset, void* out, std::size_t len) const {
    BUFFER_AUDIT();
    if (offset > size_) return result<std::size_t>::failure(error_code::out_of_range);
    if (len == 0) return result<std::size_t>::success(0);
    if (!out) return result<std::size_t>::failure(error_code::null_pointer);
    std::size_t available = size_ - offset;
    std::size_t to_copy   = len < available ? len : available;
    if (to_copy > 0) std::memmove(out, data_ + offset, to_copy);
    return result<std::size_t>::success(to_copy);
}

void buffer::fill(std::uint8_t byte) noexcept {
    BUFFER_AUDIT();
    if (size_ > 0 && data_) std::memset(data_, byte, size_);
}

}  // namespace oe
