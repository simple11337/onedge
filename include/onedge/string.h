#ifndef ONEDGE_STRING_H
#define ONEDGE_STRING_H

#include "onedge/onedge_config.h"
#include "onedge/result.h"

#include <cstddef>
#include <string_view>

namespace oe {

// Owning string, always NUL-terminated for c_str(). Move-only.
// Contents are binary safe; use view() to forward embedded NULs.
class ONEDGE_API string {
public:
    string() noexcept = default;
    ~string() noexcept;

    string(const string&)            = delete;
    string& operator=(const string&) = delete;
    string(string&& other) noexcept;
    string& operator=(string&& other) noexcept;

    // Refuses to walk past ONEDGE_MAX_ALLOC_BYTES looking for the terminator.
    ONEDGE_NODISCARD static result<string> from_cstr(const char* s);
    ONEDGE_NODISCARD static result<string> from_bytes(const char* data, std::size_t len);
    ONEDGE_NODISCARD static result<string> from_view(std::string_view sv) {
        return from_bytes(sv.data(), sv.size());
    }
    ONEDGE_NODISCARD static result<string> with_capacity(std::size_t initial_capacity);

    ONEDGE_NODISCARD result<string> clone() const;

    std::size_t size() const noexcept     { return size_; }
    std::size_t length() const noexcept   { return size_; }
    std::size_t capacity() const noexcept { return cap_; }
    bool        empty() const noexcept    { return size_ == 0; }

    // Always NUL-terminated. C readers stop at the first embedded NUL.
    const char* c_str() const noexcept { return data_ ? data_ : ""; }
    const char* data() const noexcept  { return data_ ? data_ : ""; }

    // Binary-safe; no trailing NUL.
    std::string_view view() const noexcept {
        return std::string_view(data_ ? data_ : "", size_);
    }

    ONEDGE_NODISCARD result<char> at(std::size_t i) const noexcept;

    ONEDGE_NODISCARD result<void> reserve(std::size_t new_cap);

    void clear() noexcept;
    void secure_clear() noexcept;

    ONEDGE_NODISCARD result<void> append(char c);
    ONEDGE_NODISCARD result<void> append(const char* data, std::size_t len);
    ONEDGE_NODISCARD result<void> append(std::string_view sv) { return append(sv.data(), sv.size()); }
    ONEDGE_NODISCARD result<void> append(const string& other) { return append(other.data_, other.size_); }

    ONEDGE_NODISCARD result<void> assign(std::string_view sv);
    ONEDGE_NODISCARD result<void> assign(const char* data, std::size_t len);

    // pos in [0, size()]. Safe even when sv aliases the string.
    ONEDGE_NODISCARD result<void> insert(std::size_t pos, std::string_view sv);

    // [pos, pos+length) must be in range. Resizes as needed.
    ONEDGE_NODISCARD result<void> replace(std::size_t pos, std::size_t length, std::string_view sv);

    ONEDGE_NODISCARD result<string> substr(std::size_t pos, std::size_t length) const;

    // Writes at most out_len-1 chars and a NUL. truncated when the
    // full string did not fit; out_len of 0 is invalid_argument.
    ONEDGE_NODISCARD result<std::size_t> copy_to_cstr(char* out, std::size_t out_len) const;

    ONEDGE_NODISCARD bool verify() const noexcept;

private:
    ONEDGE_NODISCARD result<void> grow_to_at_least(std::size_t needed);
    ONEDGE_NODISCARD result<void> reserve_exact(std::size_t new_cap);
    ONEDGE_NODISCARD result<void> copy_in_possibly_aliased(std::size_t dst_offset,
                                                           const char* src,
                                                           std::size_t len,
                                                           std::size_t target_size);

    char*       data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t cap_  = 0;
};

}  // namespace oe

#endif  // ONEDGE_STRING_H
