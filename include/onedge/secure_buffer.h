#ifndef ONEDGE_SECURE_BUFFER_H
#define ONEDGE_SECURE_BUFFER_H

#include "onedge/buffer.h"

namespace oe {

// buffer that secure_clears on destroy and move-assign. For secret bytes.
class secure_buffer {
public:
    secure_buffer() noexcept = default;
    ~secure_buffer() noexcept { inner_.secure_clear(); }

    secure_buffer(const secure_buffer&)            = delete;
    secure_buffer& operator=(const secure_buffer&) = delete;

    secure_buffer(secure_buffer&& other) noexcept : inner_(std::move(other.inner_)) {}

    secure_buffer& operator=(secure_buffer&& other) noexcept {
        if (this != &other) {
            inner_.secure_clear();  // zero what we're dropping
            inner_ = std::move(other.inner_);
        }
        return *this;
    }

    ONEDGE_NODISCARD static result<secure_buffer> with_capacity(std::size_t initial_capacity) {
        auto r = buffer::with_capacity(initial_capacity);
        if (!r.ok()) return result<secure_buffer>::failure(r.error());
        secure_buffer s;
        s.inner_ = std::move(r).value();
        return result<secure_buffer>::success(std::move(s));
    }

    ONEDGE_NODISCARD static result<secure_buffer> from_bytes(const void* data, std::size_t len) {
        auto r = buffer::from_bytes(data, len);
        if (!r.ok()) return result<secure_buffer>::failure(r.error());
        secure_buffer s;
        s.inner_ = std::move(r).value();
        return result<secure_buffer>::success(std::move(s));
    }

    buffer&       inner() noexcept       { return inner_; }
    const buffer& inner() const noexcept { return inner_; }

    std::size_t size() const noexcept     { return inner_.size(); }
    std::size_t capacity() const noexcept { return inner_.capacity(); }
    bool        empty() const noexcept    { return inner_.empty(); }
    const std::uint8_t* data() const noexcept { return inner_.data(); }
    std::uint8_t*       data() noexcept       { return inner_.data(); }

    ONEDGE_NODISCARD result<void> append(std::uint8_t b)                       { return inner_.append(b); }
    ONEDGE_NODISCARD result<void> append(const void* d, std::size_t n)         { return inner_.append(d, n); }
    ONEDGE_NODISCARD result<void> write(std::size_t off, const void* s, std::size_t n) {
        return inner_.write(off, s, n);
    }
    ONEDGE_NODISCARD result<std::size_t> read(std::size_t off, void* o, std::size_t n) const {
        return inner_.read(off, o, n);
    }
    ONEDGE_NODISCARD result<void> resize(std::size_t n, std::uint8_t f = 0) { return inner_.resize(n, f); }
    ONEDGE_NODISCARD result<void> reserve(std::size_t n)                    { return inner_.reserve(n); }

    void clear() noexcept        { inner_.secure_clear(); }
    void secure_clear() noexcept { inner_.secure_clear(); }

    ONEDGE_NODISCARD bool verify() const noexcept { return inner_.verify(); }

private:
    buffer inner_;
};

}  // namespace oe

#endif  // ONEDGE_SECURE_BUFFER_H
