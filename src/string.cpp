#include "onedge/string.h"
#include "onedge/checked_math.h"

#include <cstdlib>
#include <cstring>

namespace oe {

namespace {

// note: buffer.cpp has the same helper. didn't bother sharing -- if a
// third one shows up, factor it out.
constexpr std::size_t kMinGrowth = 16;

std::size_t next_capacity(std::size_t current, std::size_t needed) noexcept {
    std::size_t grown = current;
    if (grown < kMinGrowth) grown = kMinGrowth;
    auto doubled = checked_mul(grown, 2);
    if (doubled && *doubled >= needed && *doubled <= ONEDGE_MAX_ALLOC_BYTES) {
        return *doubled;
    }
    return needed;
}

// Bounded so a missing terminator can't walk past the alloc cap.
std::size_t bounded_strlen(const char* s, std::size_t cap, bool* found_terminator) noexcept {
    for (std::size_t i = 0; i < cap; ++i) {
        if (s[i] == '\0') {
            *found_terminator = true;
            return i;
        }
    }
    *found_terminator = false;
    return cap;
}

}  // namespace

bool string::verify() const noexcept {
    if (cap_ == 0) return data_ == nullptr && size_ == 0;
    if (data_ == nullptr) return false;
    if (size_ > cap_) return false;
    // The terminator must always be in place when storage exists.
    if (data_[size_] != '\0') return false;
    return true;
}

#define STRING_AUDIT()                                                           \
    do {                                                                         \
        ONEDGE_CHECK(size_ <= cap_);                                             \
        ONEDGE_CHECK(cap_ == 0 || data_ != nullptr);                             \
        ONEDGE_CHECK(cap_ != 0 || data_ == nullptr);                             \
        ONEDGE_CHECK(data_ == nullptr || data_[size_] == '\0');                  \
    } while (0)

string::~string() noexcept {
    std::free(data_);
}

string::string(string&& other) noexcept
    : data_(other.data_), size_(other.size_), cap_(other.cap_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.cap_  = 0;
}

string& string::operator=(string&& other) noexcept {
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

result<string> string::with_capacity(std::size_t initial_capacity) {
    string s;
    if (initial_capacity == 0) return result<string>::success(std::move(s));
    auto r = s.reserve_exact(initial_capacity);
    if (!r.ok()) return result<string>::failure(r.error());
    return result<string>::success(std::move(s));
}

result<string> string::from_bytes(const char* data, std::size_t len) {
    if (len == 0) return result<string>::success(string{});
    if (!data) return result<string>::failure(error_code::null_pointer);
    auto r = with_capacity(len);
    if (!r.ok()) return r;
    string s = std::move(r).value();
    std::memcpy(s.data_, data, len);  // fresh alloc
    s.size_ = len;
    s.data_[s.size_] = '\0';
    return result<string>::success(std::move(s));
}

result<string> string::from_cstr(const char* s) {
    if (!s) return result<string>::failure(error_code::null_pointer);
    bool found = false;
    std::size_t n = bounded_strlen(s, ONEDGE_MAX_ALLOC_BYTES, &found);
    if (!found) return result<string>::failure(error_code::not_null_terminated);
    return from_bytes(s, n);
}

result<string> string::clone() const {
    STRING_AUDIT();
    return from_bytes(data_, size_);
}

result<char> string::at(std::size_t i) const noexcept {
    STRING_AUDIT();
    if (i >= size_) return result<char>::failure(error_code::out_of_range);
    return result<char>::success(data_[i]);
}

result<void> string::reserve_exact(std::size_t new_cap) {
    if (new_cap <= cap_) return result<void>::success();
    auto alloc_size_opt = checked_add(new_cap, 1);
    if (!alloc_size_opt) return result<void>::failure(error_code::overflow);
    if (*alloc_size_opt > ONEDGE_MAX_ALLOC_BYTES) {
        return result<void>::failure(error_code::capacity_exceeded);
    }
    void* p = std::realloc(data_, *alloc_size_opt);
    if (!p) return result<void>::failure(error_code::out_of_memory);
    data_ = static_cast<char*>(p);
    cap_  = new_cap;
    data_[size_] = '\0';
    return result<void>::success();
}

result<void> string::reserve(std::size_t new_cap) {
    STRING_AUDIT();
    return reserve_exact(new_cap);
}

result<void> string::grow_to_at_least(std::size_t needed) {
    if (needed <= cap_) return result<void>::success();
    std::size_t target = next_capacity(cap_, needed);
    if (target < needed) target = needed;
    return reserve_exact(target);
}

void string::clear() noexcept {
    STRING_AUDIT();
    size_ = 0;
    if (data_) data_[0] = '\0';
}

void string::secure_clear() noexcept {
    STRING_AUDIT();
    if (data_) detail::secure_zero(data_, size_);
    size_ = 0;
    if (data_) data_[0] = '\0';
}

result<void> string::copy_in_possibly_aliased(std::size_t dst_offset,
                                              const char* src,
                                              std::size_t len,
                                              std::size_t target_size) {
    // Snapshot alias state before grow can invalidate src.
    bool        aliased    = detail::pointer_in_range(src, data_, cap_);
    std::size_t src_offset = aliased ? static_cast<std::size_t>(src - data_) : 0;
    auto r = grow_to_at_least(target_size);
    if (!r.ok()) return r;
    const char* effective_src = aliased ? (data_ + src_offset) : src;
    if (len > 0) std::memmove(data_ + dst_offset, effective_src, len);
    return result<void>::success();
}

result<void> string::append(char c) {
    STRING_AUDIT();
    auto new_size_opt = checked_add(size_, 1);
    if (!new_size_opt) return result<void>::failure(error_code::overflow);
    auto r = grow_to_at_least(*new_size_opt);
    if (!r.ok()) return r;
    data_[size_] = c;
    size_ = *new_size_opt;
    data_[size_] = '\0';
    return result<void>::success();
}

result<void> string::append(const char* data, std::size_t len) {
    STRING_AUDIT();
    if (len == 0) return result<void>::success();
    if (!data) return result<void>::failure(error_code::null_pointer);
    auto new_size_opt = checked_add(size_, len);
    if (!new_size_opt) return result<void>::failure(error_code::overflow);
    if (auto r = copy_in_possibly_aliased(size_, data, len, *new_size_opt); !r.ok()) return r;
    size_ = *new_size_opt;
    data_[size_] = '\0';
    return result<void>::success();
}

result<void> string::assign(std::string_view sv) {
    return assign(sv.data(), sv.size());
}

result<void> string::assign(const char* data, std::size_t len) {
    STRING_AUDIT();
    if (len == 0) {
        clear();
        return result<void>::success();
    }
    if (!data) return result<void>::failure(error_code::null_pointer);
    auto r = copy_in_possibly_aliased(0, data, len, len);
    if (!r.ok()) return r;
    size_ = len;
    data_[size_] = '\0';
    return result<void>::success();
}

result<void> string::insert(std::size_t pos, std::string_view sv) {
    STRING_AUDIT();
    if (pos > size_) return result<void>::failure(error_code::out_of_range);
    if (sv.empty()) return result<void>::success();
    // TODO: fast path when pos == size_, just call append. Not bothering yet.
    auto new_size_opt = checked_add(size_, sv.size());
    if (!new_size_opt) return result<void>::failure(error_code::overflow);

    bool        aliased    = detail::pointer_in_range(sv.data(), data_, cap_);
    std::size_t src_offset = 0;
    if (aliased) {
        src_offset = static_cast<std::size_t>(sv.data() - data_);
    }

    auto r = grow_to_at_least(*new_size_opt);
    if (!r.ok()) return r;

    // Shift the tail right first. memmove handles the overlap with the
    // unshifted tail. After this, src may point into the shifted region
    // if it was aliased and overlapped the tail, so recompute the
    // effective src position carefully.
    std::size_t tail = size_ - pos;
    if (tail > 0) std::memmove(data_ + pos + sv.size(), data_ + pos, tail);

    // If the original src sat at or past pos, the shift moved its bytes
    // by sv.size(). Adjust the recomputed source accordingly.
    const char* effective_src;
    if (aliased) {
        std::size_t eff_offset = src_offset >= pos ? src_offset + sv.size() : src_offset;
        effective_src = data_ + eff_offset;
    } else {
        effective_src = sv.data();
    }
    std::memmove(data_ + pos, effective_src, sv.size());

    size_ = *new_size_opt;
    data_[size_] = '\0';
    return result<void>::success();
}

result<void> string::replace(std::size_t pos, std::size_t length, std::string_view sv) {
    STRING_AUDIT();
    if (!range_in_bounds(pos, length, size_)) {
        auto end = checked_add(pos, length);
        if (!end) return result<void>::failure(error_code::overflow);
        return result<void>::failure(error_code::out_of_range);
    }
    auto remaining_opt = checked_sub(size_, length);
    if (!remaining_opt) return result<void>::failure(error_code::overflow);
    auto new_size_opt = checked_add(*remaining_opt, sv.size());
    if (!new_size_opt) return result<void>::failure(error_code::overflow);

    // Aliasing snapshot before growth.
    bool        aliased    = detail::pointer_in_range(sv.data(), data_, cap_);
    std::size_t src_offset = aliased
        ? static_cast<std::size_t>(sv.data() - data_)
        : 0;

    auto r = grow_to_at_least(*new_size_opt);
    if (!r.ok()) return r;

    // Move tail to its new spot first so grow/shrink both work.
    std::size_t tail_offset = pos + length;
    std::size_t tail_len    = size_ - tail_offset;
    if (tail_len > 0) {
        std::memmove(data_ + pos + sv.size(), data_ + tail_offset, tail_len);
    }

    // If aliased src lived at/past the tail, its bytes shifted too.
    const char* effective_src;
    if (aliased) {
        std::size_t eff_offset = src_offset;
        if (src_offset >= tail_offset) {
            if (sv.size() >= length) eff_offset += sv.size() - length;
            else                     eff_offset -= length - sv.size();
        }
        effective_src = data_ + eff_offset;
    } else {
        effective_src = sv.data();
    }

    if (sv.size() > 0) std::memmove(data_ + pos, effective_src, sv.size());

    size_ = *new_size_opt;
    data_[size_] = '\0';
    return result<void>::success();
}

result<string> string::substr(std::size_t pos, std::size_t length) const {
    STRING_AUDIT();
    if (!range_in_bounds(pos, length, size_)) {
        auto end = checked_add(pos, length);
        if (!end) return result<string>::failure(error_code::overflow);
        return result<string>::failure(error_code::out_of_range);
    }
    return from_bytes(data_ ? data_ + pos : nullptr, length);
}

result<std::size_t> string::copy_to_cstr(char* out, std::size_t out_len) const {
    STRING_AUDIT();
    if (out_len == 0) return result<std::size_t>::failure(error_code::invalid_argument);
    if (!out) return result<std::size_t>::failure(error_code::null_pointer);
    std::size_t to_copy = size_ < (out_len - 1) ? size_ : (out_len - 1);
    if (to_copy > 0 && data_) std::memmove(out, data_, to_copy);
    out[to_copy] = '\0';
    if (to_copy < size_) {
        return result<std::size_t>::failure(error_code::truncated);
    }
    return result<std::size_t>::success(to_copy);
}

}  // namespace oe
