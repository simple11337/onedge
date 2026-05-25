// Length-prefixed message parser. The kind of code that, written by hand
// in C, has produced dozens of CVEs over the years: integer overflow in
// the length math, OOB reads when the declared length exceeds the actual
// input, missing checks for short headers.
//
// Frame layout: [4-byte big-endian length][payload bytes...]

#include "onedge/buffer.h"
#include "onedge/checked_math.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

oe::result<oe::buffer> parse_framed_message(const std::uint8_t* input,
                                            std::size_t          input_len) {
    if (input_len < 4) {
        return oe::result<oe::buffer>::failure(oe::error_code::parse_error,
                                               "frame header truncated");
    }

    std::uint32_t declared =
        (static_cast<std::uint32_t>(input[0]) << 24) |
        (static_cast<std::uint32_t>(input[1]) << 16) |
        (static_cast<std::uint32_t>(input[2]) <<  8) |
        (static_cast<std::uint32_t>(input[3]));

    // Checked add catches the classic bug: a hostile peer sends a length
    // close to UINT32_MAX so that header_size + declared wraps to a tiny
    // value, then a hand-rolled `if (header_size + declared <= input_len)`
    // passes and the memcpy reads way past `input`.
    auto end_opt = oe::checked_add(std::size_t{4}, static_cast<std::size_t>(declared));
    if (!end_opt) {
        return oe::result<oe::buffer>::failure(oe::error_code::overflow,
                                               "length field overflowed");
    }
    if (*end_opt > input_len) {
        return oe::result<oe::buffer>::failure(oe::error_code::parse_error,
                                               "declared length exceeds input");
    }

    // from_bytes also caps against ONEDGE_MAX_ALLOC_BYTES, so even a
    // valid declared length cannot trick us into a multi-GB allocation.
    return oe::buffer::from_bytes(input + 4, declared);
}

static void try_parse(const char* label, const std::uint8_t* p, std::size_t n) {
    auto r = parse_framed_message(p, n);
    if (r.ok()) {
        std::printf("%-24s OK, payload size = %zu\n", label, r.value().size());
    } else {
        std::printf("%-24s rejected: %s\n", label, r.message());
    }
}

int main() {
    // Well-formed frame: length=3, payload "hi!"
    const std::uint8_t good[] = {0x00, 0x00, 0x00, 0x03, 'h', 'i', '!'};
    try_parse("well-formed", good, sizeof(good));

    // Truncated header (3 bytes instead of 4).
    const std::uint8_t short_header[] = {0x00, 0x00, 0x01};
    try_parse("short header", short_header, sizeof(short_header));

    // Declared length larger than input. Classic OOB-read setup.
    const std::uint8_t lies_about_size[] = {0x00, 0x00, 0xFF, 0xFF, 'a', 'b'};
    try_parse("lies about size", lies_about_size, sizeof(lies_about_size));

    // Maximum 32-bit length. header(4) + UINT32_MAX overflows on 32-bit
    // size_t builds, gets caught here. On 64-bit it gets rejected against
    // input_len instead. Either way, no OOB.
    const std::uint8_t max_length_field[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00};
    try_parse("max length field", max_length_field, sizeof(max_length_field));

    // Zero-length payload is legitimate.
    const std::uint8_t empty_payload[] = {0x00, 0x00, 0x00, 0x00};
    try_parse("empty payload", empty_payload, sizeof(empty_payload));

    return 0;
}
