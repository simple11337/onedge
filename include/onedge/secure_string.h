#ifndef ONEDGE_SECURE_STRING_H
#define ONEDGE_SECURE_STRING_H

#include "onedge/string.h"

namespace oe {

// string that secure_clears on destroy and move-assign. clear() also zeros.
class secure_string {
public:
    secure_string() noexcept = default;
    ~secure_string() noexcept { inner_.secure_clear(); }

    secure_string(const secure_string&)            = delete;
    secure_string& operator=(const secure_string&) = delete;

    secure_string(secure_string&& other) noexcept : inner_(std::move(other.inner_)) {}

    secure_string& operator=(secure_string&& other) noexcept {
        if (this != &other) {
            inner_.secure_clear();
            inner_ = std::move(other.inner_);
        }
        return *this;
    }

    ONEDGE_NODISCARD static result<secure_string> from_cstr(const char* s) {
        auto r = string::from_cstr(s);
        if (!r.ok()) return result<secure_string>::failure(r.error());
        secure_string out;
        out.inner_ = std::move(r).value();
        return result<secure_string>::success(std::move(out));
    }

    ONEDGE_NODISCARD static result<secure_string> from_bytes(const char* data, std::size_t len) {
        auto r = string::from_bytes(data, len);
        if (!r.ok()) return result<secure_string>::failure(r.error());
        secure_string out;
        out.inner_ = std::move(r).value();
        return result<secure_string>::success(std::move(out));
    }

    string&       inner() noexcept       { return inner_; }
    const string& inner() const noexcept { return inner_; }

    std::size_t      size() const noexcept     { return inner_.size(); }
    std::size_t      capacity() const noexcept { return inner_.capacity(); }
    bool             empty() const noexcept    { return inner_.empty(); }
    const char*      c_str() const noexcept    { return inner_.c_str(); }
    std::string_view view() const noexcept     { return inner_.view(); }

    ONEDGE_NODISCARD result<void> append(char c)                                 { return inner_.append(c); }
    ONEDGE_NODISCARD result<void> append(const char* d, std::size_t n)           { return inner_.append(d, n); }
    ONEDGE_NODISCARD result<void> append(std::string_view sv)                    { return inner_.append(sv); }
    ONEDGE_NODISCARD result<void> assign(std::string_view sv)                    { return inner_.assign(sv); }
    ONEDGE_NODISCARD result<void> reserve(std::size_t n)                         { return inner_.reserve(n); }
    ONEDGE_NODISCARD result<std::size_t> copy_to_cstr(char* o, std::size_t n) const {
        return inner_.copy_to_cstr(o, n);
    }

    void clear() noexcept        { inner_.secure_clear(); }
    void secure_clear() noexcept { inner_.secure_clear(); }

    ONEDGE_NODISCARD bool verify() const noexcept { return inner_.verify(); }

private:
    string inner_;
};

}  // namespace oe

#endif  // ONEDGE_SECURE_STRING_H
