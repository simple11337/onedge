#ifndef ONEDGE_READER_H
#define ONEDGE_READER_H

#include "onedge/onedge_config.h"
#include "onedge/result.h"
#include "onedge/span.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace oe {

// Sequential reader over a byte view. Short reads return out_of_range
// and leave the cursor where it was. BE everywhere except u32 (one
// format we deal with is LE, the rest are network order).
class reader {
public:
    constexpr reader() noexcept = default;
    constexpr explicit reader(const_byte_span data) noexcept : data_(data) {}
    constexpr reader(const void* data, std::size_t size) noexcept
        : data_(const_byte_span(static_cast<const std::uint8_t*>(data), size)) {}

    constexpr std::size_t position() const noexcept  { return pos_; }
    constexpr std::size_t remaining() const noexcept { return data_.size() - pos_; }
    constexpr bool        empty() const noexcept     { return pos_ >= data_.size(); }

    ONEDGE_NODISCARD result<void> skip(std::size_t n) noexcept {
        if (n > remaining()) return result<void>::failure(error_code::out_of_range);
        pos_ += n;
        return result<void>::success();
    }

    ONEDGE_NODISCARD result<void> read_bytes(void* out, std::size_t len) noexcept {
        if (len == 0) return result<void>::success();
        if (!out) return result<void>::failure(error_code::null_pointer);
        if (len > remaining()) return result<void>::failure(error_code::out_of_range);
        std::memcpy(out, data_.data() + pos_, len);
        pos_ += len;
        return result<void>::success();
    }

    // Borrows; valid until the reader's source goes away.
    ONEDGE_NODISCARD result<const_byte_span> read_view(std::size_t len) noexcept {
        if (len > remaining()) return result<const_byte_span>::failure(error_code::out_of_range);
        const_byte_span out(data_.data() + pos_, len);
        pos_ += len;
        return result<const_byte_span>::success(out);
    }

    ONEDGE_NODISCARD result<const_byte_span> peek_view(std::size_t len) const noexcept {
        if (len > remaining()) return result<const_byte_span>::failure(error_code::out_of_range);
        return result<const_byte_span>::success(const_byte_span(data_.data() + pos_, len));
    }

    ONEDGE_NODISCARD result<std::uint8_t> read_u8() noexcept {
        if (remaining() < 1) return result<std::uint8_t>::failure(error_code::out_of_range);
        std::uint8_t v = data_[pos_];
        pos_ += 1;
        return result<std::uint8_t>::success(v);
    }

    ONEDGE_NODISCARD result<std::uint16_t> read_u16_be() noexcept {
        if (remaining() < 2) return result<std::uint16_t>::failure(error_code::out_of_range);
        std::uint16_t v = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(data_[pos_])     << 8) |
             static_cast<std::uint16_t>(data_[pos_ + 1]));
        pos_ += 2;
        return result<std::uint16_t>::success(v);
    }

    ONEDGE_NODISCARD result<std::uint32_t> read_u32_be() noexcept {
        if (remaining() < 4) return result<std::uint32_t>::failure(error_code::out_of_range);
        std::uint32_t v =
            (static_cast<std::uint32_t>(data_[pos_])     << 24) |
            (static_cast<std::uint32_t>(data_[pos_ + 1]) << 16) |
            (static_cast<std::uint32_t>(data_[pos_ + 2]) <<  8) |
             static_cast<std::uint32_t>(data_[pos_ + 3]);
        pos_ += 4;
        return result<std::uint32_t>::success(v);
    }

    ONEDGE_NODISCARD result<std::uint32_t> read_u32_le() noexcept {
        if (remaining() < 4) return result<std::uint32_t>::failure(error_code::out_of_range);
        std::uint32_t v =
             static_cast<std::uint32_t>(data_[pos_])           |
            (static_cast<std::uint32_t>(data_[pos_ + 1]) <<  8) |
            (static_cast<std::uint32_t>(data_[pos_ + 2]) << 16) |
            (static_cast<std::uint32_t>(data_[pos_ + 3]) << 24);
        pos_ += 4;
        return result<std::uint32_t>::success(v);
    }

    ONEDGE_NODISCARD result<std::uint64_t> read_u64_be() noexcept {
        if (remaining() < 8) return result<std::uint64_t>::failure(error_code::out_of_range);
        std::uint64_t v = 0;
        for (int i = 0; i < 8; ++i) {
            v = (v << 8) | static_cast<std::uint64_t>(data_[pos_ + static_cast<std::size_t>(i)]);
        }
        pos_ += 8;
        return result<std::uint64_t>::success(v);
    }

private:
    const_byte_span data_{};
    std::size_t     pos_ = 0;
};

}  // namespace oe

#endif  // ONEDGE_READER_H
