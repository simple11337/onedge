#ifndef ONEDGE_RESULT_H
#define ONEDGE_RESULT_H

#include "onedge/onedge_config.h"
#include "onedge/error.h"

#include <new>
#include <type_traits>
#include <utility>

namespace oe {

// Error info. Static-storage message only (we don't allocate here).
struct error_info {
    error_code  code    = error_code::ok;
    const char* message = nullptr;
};

namespace detail {
inline const char* default_message(error_code c) noexcept {
    return error_message(c);
}
}  // namespace detail

// Either a T or an error_info. [[nodiscard]] so you can't drop failures.
// Copy/copy-assign only get instantiated if you actually copy, so move-only T works.
template <typename T>
class ONEDGE_NODISCARD result {
public:
    using value_type = T;

    static result success(const T& v) {
        result r;
        r.construct_value(v);
        return r;
    }
    static result success(T&& v) {
        result r;
        r.construct_value(std::move(v));
        return r;
    }
    static result failure(error_code code, const char* message = nullptr) {
        result r;
        r.err_ = error_info{code, message ? message : detail::default_message(code)};
        r.has_value_ = false;
        return r;
    }
    static result failure(error_info info) {
        result r;
        r.err_ = info;
        if (!r.err_.message) r.err_.message = detail::default_message(info.code);
        r.has_value_ = false;
        return r;
    }

    result(const result& other) : has_value_(other.has_value_) {
        if (has_value_) construct_value(other.value_ref());
        else err_ = other.err_;
    }
    result(result&& other) noexcept(std::is_nothrow_move_constructible<T>::value)
        : has_value_(other.has_value_) {
        if (has_value_) construct_value(std::move(other.value_ref()));
        else err_ = other.err_;
    }
    result& operator=(const result& other) {
        if (this != &other) {
            destroy_value();
            has_value_ = other.has_value_;
            if (has_value_) construct_value(other.value_ref());
            else err_ = other.err_;
        }
        return *this;
    }
    result& operator=(result&& other) noexcept(std::is_nothrow_move_constructible<T>::value) {
        if (this != &other) {
            destroy_value();
            has_value_ = other.has_value_;
            if (has_value_) construct_value(std::move(other.value_ref()));
            else err_ = other.err_;
        }
        return *this;
    }
    ~result() { destroy_value(); }

    bool ok() const noexcept { return has_value_; }
    explicit operator bool() const noexcept { return has_value_; }

    error_code code() const noexcept { return has_value_ ? error_code::ok : err_.code; }
    const char* message() const noexcept {
        return has_value_ ? "" : (err_.message ? err_.message : detail::default_message(err_.code));
    }
    error_info error() const noexcept { return has_value_ ? error_info{} : err_; }

    // UB if !ok(). Check first or use value_or().
    T& value() & { return value_ref(); }
    const T& value() const& { return value_ref(); }
    T&& value() && { return std::move(value_ref()); }

    T value_or(T fallback) const& { return has_value_ ? value_ref() : fallback; }
    T value_or(T fallback) && {
        return has_value_ ? std::move(value_ref()) : fallback;
    }

private:
    result() : has_value_(false) {}

    void construct_value(const T& v) {
        ::new (storage()) T(v);
        has_value_ = true;
    }
    void construct_value(T&& v) {
        ::new (storage()) T(std::move(v));
        has_value_ = true;
    }
    void destroy_value() {
        if (has_value_) {
            value_ref().~T();
            has_value_ = false;
        }
    }
    void* storage() noexcept { return static_cast<void*>(&storage_); }
    T& value_ref() noexcept { return *std::launder(reinterpret_cast<T*>(&storage_)); }
    const T& value_ref() const noexcept {
        return *std::launder(reinterpret_cast<const T*>(&storage_));
    }

    typename std::aligned_storage<sizeof(T), alignof(T)>::type storage_;
    error_info err_{};
    bool       has_value_ = false;
};

// Void specialization.
template <>
class ONEDGE_NODISCARD result<void> {
public:
    using value_type = void;

    static result success() { return result{}; }
    static result failure(error_code code, const char* message = nullptr) {
        result r;
        r.err_ = error_info{code, message ? message : detail::default_message(code)};
        r.has_value_ = false;
        return r;
    }
    static result failure(error_info info) {
        result r;
        r.err_ = info;
        if (!r.err_.message) r.err_.message = detail::default_message(info.code);
        r.has_value_ = false;
        return r;
    }

    bool ok() const noexcept { return has_value_; }
    explicit operator bool() const noexcept { return has_value_; }
    error_code code() const noexcept { return has_value_ ? error_code::ok : err_.code; }
    const char* message() const noexcept {
        return has_value_ ? "" : (err_.message ? err_.message : detail::default_message(err_.code));
    }
    error_info error() const noexcept { return has_value_ ? error_info{} : err_; }

private:
    result() : has_value_(true) {}
    error_info err_{};
    bool       has_value_ = true;
};

}  // namespace oe

#endif  // ONEDGE_RESULT_H
