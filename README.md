# onedge

Bounds-checked string and buffer types for C and C++17. Refuses out-of-bounds writes, integer overflow in size math, silent truncation, and aliased copies that dangle after a realloc. No exceptions.

Most heap-corruption bugs in C and C++ trace back to the same handful of patterns. A length field is wrong and `memcpy` walks off the end. `size + n` wraps to zero, `malloc(0)` succeeds, and the next write is anywhere. `strncpy` truncates without a NUL. A pointer into a buffer survives a `realloc` and now reads from a freed block. onedge is the library you grab to make those patterns not apply to your code.

```cpp
#include "onedge/string.h"

auto r = oe::string::from_cstr(user_input);  // bounded; no walk-forever
if (!r.ok()) return r.code();
auto s = std::move(r).value();
if (auto a = s.append(", world"); !a.ok()) return a.code();
```

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

ASAN and UBSAN on Linux:

```sh
cmake -S . -B build -DONEDGE_SANITIZERS=ON
cmake --build build && ctest --test-dir build --output-on-failure
```

Strict warnings as errors with `-DONEDGE_WERROR=ON`. Defaults: invariant guards on in release too, single-allocation cap at 1 GiB. Override `ONEDGE_MAX_ALLOC_BYTES` if you need more.

## Worked example

Length-prefixed message parsers are the bug pattern. In C you usually see one of three mistakes: no integer-overflow check on `header_size + declared`, trusting the declared length against the actual input, or copying without a bounds check. With `oe::reader`:

```cpp
#include "onedge/reader.h"
#include "onedge/buffer.h"

oe::result<oe::buffer> parse(const std::uint8_t* input, std::size_t n) {
    oe::reader r(input, n);
    auto len = r.read_u32_be();
    if (!len.ok())
        return oe::result<oe::buffer>::failure(len.code(), "short header");
    auto body = r.read_view(len.value());
    if (!body.ok())
        return oe::result<oe::buffer>::failure(body.code(), "declared length exceeds input");
    return oe::buffer::from_bytes(body.value().data(), body.value().size());
}
```

`read_u32_be` refuses to advance past the end. `read_view` refuses if the declared length doesn't fit. `from_bytes` caps the allocation against `ONEDGE_MAX_ALLOC_BYTES`. There's no place to drop a bounds check.

Full version with hostile inputs and rejected outputs in [`examples/parse_framed_message.cpp`](examples/parse_framed_message.cpp).

## What's in here

- `oe::buffer` heap byte buffer. Self-append works across a realloc.
- `oe::string` heap, always NUL-terminated, binary safe via `view()`.
- `oe::fixed_buffer<N>` stack buffer of fixed `N`, same checks, no heap.
- `oe::secure_buffer` / `oe::secure_string` zero on destroy. For secrets.
- `oe::span<T>` non-owning view, bounded slicing. Fills in for `std::span` on C++17.
- `oe::reader` / `oe::writer` bounded binary I/O. BE for every size, plus LE for `u32`.
- `oe::result<T>` `[[nodiscard]]` result type with stable error codes.
- `oe::checked_add` / `checked_sub` / `checked_mul` / `checked_grow` size math that returns `nullopt` on overflow.
- `ONEDGE_CHECK` invariant guard. Aborts the process if a container's internal state has been corrupted from outside. On by default in release.
- `onedge/c_api.h` C ABI with opaque handles and stable `oe_status` codes.

## Integrating

```cmake
add_subdirectory(third_party/onedge)
target_link_libraries(your_app PRIVATE onedge)
```

Or copy `include/onedge/` plus the four files in `src/` into your tree. The headers don't pull anything from `src/` for the templates and inline code, so spans, reader, writer, fixed_buffer, and the secure wrappers work header-only.

## Bug classes it targets

| Bug | Where it gets caught |
|---|---|
| OOB write | `buffer::write`, `string::insert`, `string::replace`, every fixed_buffer mutation |
| OOB read | `at`, `read`, `view`, `substr`, reader integer reads |
| `size + n` overflow | `checked_add` before any allocation; refuses with `error_code::overflow` |
| Multi-GB malloc from a flipped length byte | `ONEDGE_MAX_ALLOC_BYTES` cap on every allocation path |
| Silent truncation | `copy_to_cstr` returns `truncated`; never silently chops |
| Missing NUL | `from_cstr` does a bounded scan, returns `not_null_terminated` |
| Aliased src dangling after realloc | append/insert/replace/assign snapshot the offset before grow and recompute after |
| Overlapping copies | mutating ops use `memmove`, not `memcpy` |
| Secrets in freed heap blocks | `secure_clear`, `secure_buffer`, `secure_string` |
| Ignored failure | every fallible API is `[[nodiscard]]` |
| Heap corruption from elsewhere | `ONEDGE_CHECK` audits invariants on entry, aborts on violation |

## Not in here yet

- LE variants for `u16` and `u64` in the reader and writer. I only needed LE for one specific format that uses `u32`, so the others aren't written. PRs welcome.
- A `format` helper. printf-style is too easy to mess up the API on. Haven't designed one I like.
- A real fuzz harness. The test suite covers edge cases by hand but a libFuzzer entry point would be the next thing.
- A `fixed_string<N>` to match `fixed_buffer<N>`. Most callers seem fine with `oe::string`.

## What it isn't

Not a sandbox. Code that calls `memcpy` on raw pointers around onedge can still corrupt memory. Not an allocator. Doesn't fix logic bugs. Doesn't try to be the fastest string library. Not thread safe per instance; two separate containers in two threads are independent.

## License

[LICENSE](LICENSE).
