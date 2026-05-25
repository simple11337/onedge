#ifndef ONEDGE_BUFFER_H
#define ONEDGE_BUFFER_H

#include "onedge/onedge_config.h"
#include "onedge/result.h"

#include <cstddef>
#include <cstdint>

namespace oe {

// Owning byte buffer. Move-only; clone() makes alloc failure visible.
// Self-append and self-write are safe across growth.
class ONEDGE_API buffer {
public:
    buffer() noexcept = default;
    ~buffer() noexcept;

    buffer(const buffer&)            = delete;
    buffer& operator=(const buffer&) = delete;
    buffer(buffer&& other) noexcept;
    buffer& operator=(buffer&& other) noexcept;

    ONEDGE_NODISCARD static result<buffer> with_capacity(std::size_t initial_capacity);
    ONEDGE_NODISCARD static result<buffer> from_bytes(const void* data, std::size_t len);

    ONEDGE_NODISCARD result<buffer> clone() const;

    std::size_t size() const noexcept     { return size_; }
    std::size_t capacity() const noexcept { return cap_; }
    bool        empty() const noexcept    { return size_ == 0; }

    // Invalidated by anything that may grow (reserve, resize, append).
    const std::uint8_t* data() const noexcept { return data_; }
    std::uint8_t*       data() noexcept       { return data_; }

    ONEDGE_NODISCARD result<std::uint8_t> at(std::size_t i) const noexcept;
    ONEDGE_NODISCARD result<void>         set(std::size_t i, std::uint8_t value) noexcept;

    ONEDGE_NODISCARD result<void> reserve(std::size_t new_cap);
    ONEDGE_NODISCARD result<void> resize(std::size_t new_size, std::uint8_t fill_byte = 0);

    void clear() noexcept;
    // Zero before clearing. Use when the buffer held secrets.
    void secure_clear() noexcept;

    ONEDGE_NODISCARD result<void> append(std::uint8_t byte);
    ONEDGE_NODISCARD result<void> append(const void* data, std::size_t len);

    // No growth; offset + len must already fit in size().
    ONEDGE_NODISCARD result<void> write(std::size_t offset, const void* src, std::size_t len);

    // Returns bytes copied, clamped to what's available.
    ONEDGE_NODISCARD result<std::size_t> read(std::size_t offset, void* out, std::size_t len) const;

    void fill(std::uint8_t byte) noexcept;

    // True if internal state is consistent.
    ONEDGE_NODISCARD bool verify() const noexcept;

private:
    ONEDGE_NODISCARD result<void> grow_to_at_least(std::size_t needed);
    ONEDGE_NODISCARD result<void> copy_in_possibly_aliased(std::size_t dst_offset,
                                                           const void* src,
                                                           std::size_t len);

    std::uint8_t* data_ = nullptr;
    std::size_t   size_ = 0;
    std::size_t   cap_  = 0;
};

}  // namespace oe

#endif  // ONEDGE_BUFFER_H
