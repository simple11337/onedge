# onedge

A small C++ library with no external deps. It cuts down the common causes of memory corruption and RCE bugs in C and C++ code: buffer overflows, out-of-bounds access, integer overflow in size math, silent truncation, missing null terminators, ignored error returns, aliased copies that dangle after a realloc.

It does not try to fix every security problem. It removes the exploit paths that keep showing up in real code.

## What it gives you

Owning containers:
- `oe::buffer` — heap byte buffer with bounds-checked read, write, resize, append; alias-safe copies; `secure_clear` for sensitive content
- `oe::string` — heap, always-null-terminated string with safe append, assign, insert, replace, substr, explicit truncation, and alias-safe self-mutation
- `oe::fixed_buffer<N>` — stack-allocated bounds-checked buffer; no heap, no growth, capacity fixed at compile time
- `oe::secure_buffer` / `oe::secure_string` — same as their counterparts but zero on destruction and on move-assign

Views and streams:
- `oe::span<T>` / `oe::byte_span` / `oe::const_byte_span` — non-owning bounds-checked view; bounded `subspan`, capped `first`/`last`
- `oe::reader` — bounded sequential reader; `read_u8`, `read_u{16,32,64}_be`, `read_u32_le`, plus `peek_view`, `skip`, `read_view`. LE for u16/u64 not implemented yet (haven't needed them).
- `oe::writer` — bounded sequential writer over an `oe::buffer`, same endian coverage as the reader

Error handling and math:
- `oe::result<T>` — non-throwing result type with stable error codes, marked `[[nodiscard]]` so callers cannot drop failures on the floor
- `oe::checked_add` / `oe::checked_sub` / `oe::checked_mul` / `oe::checked_grow` / `oe::range_in_bounds` / `oe::checked_cast` — checked size and integer math, allocation-free on the error path

Runtime hardening:
- `ONEDGE_CHECK` invariant guard that aborts on internal corruption rather than letting a bad state become an exploit primitive
- `oe::detail::secure_zero` — volatile-loop zeroing the optimizer cannot fold away

C interop:
- `onedge/c_api.h` — minimal C-compatible layer with opaque handles, stable status codes, and the same security model

## Threat model

The library assumes:

- input length, content, and offsets may all be attacker controlled
- input may be malformed, truncated, non-null-terminated, or binary
- callers will pass wrong sizes
- integer math will overflow
- pointers may alias the destination buffer
- a heap corruption may have already happened by the time a method is called

It is built to refuse to operate on bad input rather than to limp along.

### What it defends against

- **Buffer overflow / OOB write** — every mutating call refuses to write past the destination. `write`, `append`, `insert`, `replace`, `set`, `copy_to_cstr` all bounds-check first.
- **OOB read** — `at`, `read`, `view`, `substr` refuse to read past the end. `read` clamps to what is available and reports the actual count.
- **Integer overflow in size math** — every length, offset, and capacity calculation goes through the `checked_*` helpers. Overflow returns `error_code::overflow` before any allocation.
- **Oversized allocation** — any request larger than `ONEDGE_MAX_ALLOC_BYTES` (default 1 GiB) is refused with `capacity_exceeded`. Stops attacker-flipped length fields from triggering multi-GB allocations.
- **Silent truncation** — `copy_to_cstr` returns `truncated` and writes a NUL; it never silently chops data without telling you.
- **Non-null-terminated input** — `from_cstr` walks bounded and returns `not_null_terminated` if no terminator is found within the cap.
- **Aliased source after realloc** — `append`, `insert`, `replace`, `assign` capture the source offset before any growth and recompute the source pointer after a realloc, so passing your own buffer back as input is safe.
- **Overlapping write source and destination** — `write` and the alias-safe internal copy use `memmove` so overlapping ranges produce defined results.
- **Use after free of secret material** — `secure_clear` zeroes the bytes through a volatile path the optimizer is not allowed to fold away, before resetting the size.
- **Use after free at the API level** — owned containers are move-only and reset the source on move; copy is deleted (use `clone()`), so accidental shallow copies cannot happen.
- **Internal state corruption** — every mutating call runs `ONEDGE_CHECK` against its invariants. If a buffer or string has been corrupted out of band (heap smash, double-free elsewhere, wild pointer write), the next call hits the guard and aborts the process rather than handing the attacker a usable primitive. Public `verify()` lets callers check non-destructively.
- **Ignored error returns** — fallible APIs are `[[nodiscard]]`; on MSVC the equivalent attribute applies. Forgetting to inspect a result is a compile warning.

### What it does not solve

- It is not a sandbox. Code that calls `memcpy` on raw pointers without going through onedge can still corrupt memory.
- It is not a memory allocator. It does not replace `malloc` or guard heap metadata.
- It does not detect bugs in the caller's logic.
- It is not thread safe. A single buffer or string accessed from multiple threads concurrently needs external synchronization.
- It does not aim to be the fastest container library. Safety wins over a few cycles.
- It cannot prevent the C API caller from passing the same handle to `destroy` twice. Don't do that.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Treat warnings as errors:

```sh
cmake -S . -B build -DONEDGE_WERROR=ON
```

Sanitizers (Linux, gcc or clang):

```sh
cmake -S . -B build -DONEDGE_SANITIZERS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Disable runtime invariant checks (only do this if you've benchmarked the cost and you know you want it; the default is on in release):

```sh
cmake -S . -B build -DCMAKE_CXX_FLAGS=-DONEDGE_DISABLE_INVARIANT_CHECKS
```

## Integrate

Drop the library into your tree and link the static target:

```cmake
add_subdirectory(third_party/onedge)
target_link_libraries(your_app PRIVATE onedge)
```

Or build it once and use the headers in `include/onedge/`.

## Worked example: parsing a length-prefixed message with reader

The framed-message parser from the previous version becomes a few lines once `oe::reader` is in the mix:

```cpp
#include "onedge/reader.h"
#include "onedge/buffer.h"

oe::result<oe::buffer> parse_with_reader(const std::uint8_t* input, std::size_t n) {
    oe::reader r(input, n);
    auto len = r.read_u32_be();             // refuses if fewer than 4 bytes left
    if (!len.ok()) return oe::result<oe::buffer>::failure(len.code(), "short header");
    auto body = r.read_view(len.value());   // refuses if declared length exceeds input
    if (!body.ok()) return oe::result<oe::buffer>::failure(body.code(), "declared length exceeds input");
    return oe::buffer::from_bytes(body.value().data(), body.value().size());
}
```

The reader handles the bounds check on every step. No manual `if (pos + 4 > n)`, no manual cursor arithmetic, no integer overflow window.

## Worked example: parsing a length-prefixed message

Network protocols, file formats, and IPC channels all tend to look the same: a length field followed by a payload. Hand-rolled C parsers for that pattern have produced a steady stream of CVEs. The two recurring bugs are integer overflow in the length math (so a hostile peer wraps `header + length` to a tiny value and the check passes) and OOB reads when the declared length is larger than what actually arrived.

Here is the same parser written against onedge. The full source is in [`examples/parse_framed_message.cpp`](examples/parse_framed_message.cpp).

```cpp
#include "onedge/buffer.h"
#include "onedge/checked_math.h"

oe::result<oe::buffer> parse_framed_message(const std::uint8_t* input,
                                            std::size_t          input_len) {
    if (input_len < 4) {
        return oe::result<oe::buffer>::failure(oe::error_code::parse_error,
                                               "frame header truncated");
    }

    std::uint32_t declared =
        (std::uint32_t(input[0]) << 24) | (std::uint32_t(input[1]) << 16) |
        (std::uint32_t(input[2]) <<  8) |  std::uint32_t(input[3]);

    // Catches the wraparound bug. Without this, `4 + declared` could
    // overflow size_t on a 32-bit build and the bounds check below would
    // pass on a tiny number while memcpy then read gigabytes.
    auto end_opt = oe::checked_add(std::size_t{4}, std::size_t{declared});
    if (!end_opt)            return oe::result<oe::buffer>::failure(oe::error_code::overflow);
    if (*end_opt > input_len) return oe::result<oe::buffer>::failure(oe::error_code::parse_error,
                                                          "declared length exceeds input");

    // from_bytes refuses anything past ONEDGE_MAX_ALLOC_BYTES, so even a
    // valid declared length cannot trick us into a multi-GB malloc.
    return oe::buffer::from_bytes(input + 4, declared);
}
```

Run the example and watch each hostile input get rejected with a typed error instead of a crash:

```text
well-formed              OK, payload size = 3
short header             rejected: frame header truncated
lies about size          rejected: declared length exceeds input
max length field         rejected: declared length exceeds input
empty payload            OK, payload size = 0
```

What the library does for you here, that a hand-rolled C version usually does not:

- `checked_add` rejects the wraparound case before any allocation
- `from_bytes` caps the allocation against `ONEDGE_MAX_ALLOC_BYTES`
- the returned `result<buffer>` is `[[nodiscard]]`, so dropping the failure is a compile warning
- the resulting `oe::buffer` owns its memory; the caller cannot accidentally use the input pointer past its lifetime

## Quick taste

C++:

```cpp
#include "onedge/string.h"

auto r = oe::string::from_cstr(user_input);
if (!r.ok()) return r.code();
auto s = std::move(r).value();
if (auto a = s.append(std::string_view(suffix)); !a.ok()) return a.code();

// Safe to feed the string back into itself - no realloc-dangling.
if (auto d = s.append(s.view()); !d.ok()) return d.code();

// Clear sensitive content before scope ends.
s.secure_clear();
```

C:

```c
#include "onedge/c_api.h"

oe_string* s = NULL;
if (oe_string_from_cstr("hello", &s) != OE_OK) return 1;
if (oe_string_append_cstr(s, ", world") != OE_OK) { oe_string_destroy(s); return 1; }

char out[32];
size_t copied = 0;
oe_status rc = oe_string_copy_to_cstr(s, out, sizeof(out), &copied);
if (rc == OE_E_TRUNCATED) {
    // out still NUL-terminated, contains the prefix
}

oe_string_secure_clear(s);
oe_string_destroy(s);
```

## Bug classes it targets

| Bug class | Where it shows up here |
|---|---|
| Buffer overflow | `buffer::write`, `string::append`, `string::insert` refuse out-of-range writes |
| OOB read | `buffer::read` / `buffer::at` / `string::at` return `out_of_range` |
| Integer overflow in size math | every size add and multiply goes through `checked_*`; no allocation on overflow |
| Oversized allocation | `ONEDGE_MAX_ALLOC_BYTES` cap on every alloc path |
| Silent truncation | `string::copy_to_cstr` returns `truncated` and never silently chops data |
| Non-null-terminated input | `string::from_cstr` walks bounded and returns `not_null_terminated` |
| Aliased source dangling after grow | `append`/`insert`/`replace`/`assign` snapshot src offset, recompute after realloc |
| Overlapping copies | mutating ops use `memmove` not `memcpy` |
| Use after free of secrets | `secure_clear` zeroes through a non-optimizable path |
| Use after free (API level) | move-only owners; copy is deleted, `clone()` makes failure observable |
| Ignored error returns | every fallible API is `[[nodiscard]]` |
| Heap corruption from elsewhere | `ONEDGE_CHECK` audits state at every mutating call entry and aborts on violation |

## Testing

`tests/` has a small no-dep harness. Each public API has normal, edge, invalid-input, and aliasing cases. Run with `ctest` after configuring the build.

The test suite is sanitizer-friendly. On Linux with `-DONEDGE_SANITIZERS=ON`, ASAN and UBSAN run against every test.

## Security defaults

- Invariant checks are **on** in release builds. Turn them off only if you have measured the cost in your specific workload and decided it isn't worth the guarantee.
- The allocation cap defaults to 1 GiB. Override via `-DONEDGE_MAX_ALLOC_BYTES=<bytes>` at build time if your workload genuinely needs more.
- Strict compiler warnings are on. `ONEDGE_WERROR=ON` makes them fatal; recommended for CI.
- The library compiles cleanly under MSVC `/W4 /permissive-` and gcc/clang `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`.

## License

See [LICENSE](LICENSE).
