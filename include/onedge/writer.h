#ifndef ONEDGE_WRITER_H
#define ONEDGE_WRITER_H

#include "onedge/onedge_config.h"
#include "onedge/buffer.h"
#include "onedge/result.h"

#include <cstddef>
#include <cstdint>

namespace oe {

// Append-only sink over an oe::buffer. Holds a non-owning pointer;
// buffer must outlive the writer. Same endian-coverage as reader.
class writer {
public:
    constexpr writer() noexcept = default;
    constexpr explicit writer(buffer& out) noexcept : out_(&out) {}

    constexpr std::size_t written() const noexcept { return written_; }

    ONEDGE_NODISCARD result<void> write_bytes(const void* data, std::size_t len) {
        if (!out_) return result<void>::failure(error_code::null_pointer);
        auto r = out_->append(data, len);
        if (r.ok()) written_ += len;
        return r;
    }

    ONEDGE_NODISCARD result<void> write_u8(std::uint8_t v) {
        return write_bytes(&v, 1);
    }

    ONEDGE_NODISCARD result<void> write_u16_be(std::uint16_t v) {
        std::uint8_t buf[2] = {
            static_cast<std::uint8_t>((v >> 8) & 0xFF),
            static_cast<std::uint8_t>(v & 0xFF),
        };
        return write_bytes(buf, 2);
    }

    ONEDGE_NODISCARD result<void> write_u32_be(std::uint32_t v) {
        std::uint8_t buf[4] = {
            static_cast<std::uint8_t>((v >> 24) & 0xFF),
            static_cast<std::uint8_t>((v >> 16) & 0xFF),
            static_cast<std::uint8_t>((v >>  8) & 0xFF),
            static_cast<std::uint8_t>(v & 0xFF),
        };
        return write_bytes(buf, 4);
    }

    ONEDGE_NODISCARD result<void> write_u32_le(std::uint32_t v) {
        std::uint8_t buf[4] = {
            static_cast<std::uint8_t>(v & 0xFF),
            static_cast<std::uint8_t>((v >>  8) & 0xFF),
            static_cast<std::uint8_t>((v >> 16) & 0xFF),
            static_cast<std::uint8_t>((v >> 24) & 0xFF),
        };
        return write_bytes(buf, 4);
    }

    ONEDGE_NODISCARD result<void> write_u64_be(std::uint64_t v) {
        std::uint8_t buf[8];
        for (int i = 0; i < 8; ++i) {
            buf[i] = static_cast<std::uint8_t>((v >> ((7 - i) * 8)) & 0xFF);
        }
        return write_bytes(buf, 8);
    }

private:
    buffer*     out_     = nullptr;
    std::size_t written_ = 0;
};

}  // namespace oe

#endif  // ONEDGE_WRITER_H
