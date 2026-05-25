// onedge config: platform/lang detection, visibility macros, alloc cap,
// invariant guard, low-level safety helpers. Nothing in here throws.

#ifndef ONEDGE_CONFIG_H
#define ONEDGE_CONFIG_H

#include <cstddef>
#include <cstdint>

#if defined(__cplusplus)
#  if __cplusplus < 201703L && !(defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#    error "onedge requires C++17 or newer"
#  endif
#endif

#if defined(_WIN32)
#  define ONEDGE_PLATFORM_WINDOWS 1
#else
#  define ONEDGE_PLATFORM_WINDOWS 0
#endif

#if defined(__linux__)
#  define ONEDGE_PLATFORM_LINUX 1
#else
#  define ONEDGE_PLATFORM_LINUX 0
#endif

#if defined(ONEDGE_BUILD_SHARED)
#  if defined(_MSC_VER)
#    if defined(ONEDGE_BUILDING)
#      define ONEDGE_API __declspec(dllexport)
#    else
#      define ONEDGE_API __declspec(dllimport)
#    endif
#  else
#    define ONEDGE_API __attribute__((visibility("default")))
#  endif
#else
#  define ONEDGE_API
#endif

#if defined(__cplusplus) && __cplusplus >= 201703L
#  define ONEDGE_NODISCARD [[nodiscard]]
#elif defined(_MSC_VER) && _MSC_VER >= 1700
#  define ONEDGE_NODISCARD _Check_return_
#elif defined(__GNUC__) || defined(__clang__)
#  define ONEDGE_NODISCARD __attribute__((warn_unused_result))
#else
#  define ONEDGE_NODISCARD
#endif

// Cap any single allocation. A flipped length byte shouldn't be able to
// turn into a multi-GB malloc. Override at build time if you actually
// need more.
#ifndef ONEDGE_MAX_ALLOC_BYTES
#  define ONEDGE_MAX_ALLOC_BYTES (static_cast<std::size_t>(1) << 30)
#endif

#if defined(__cplusplus)

namespace oe {
namespace detail {

// Hard fail. State's already wrong by the time we get here.
[[noreturn]] ONEDGE_API void invariant_failure(const char* expr,
                                               const char* file,
                                               int line) noexcept;

// Volatile zero loop the optimizer can't drop.
inline void secure_zero(void* p, std::size_t n) noexcept {
    if (!p || n == 0) return;
    volatile unsigned char* vp = static_cast<volatile unsigned char*>(p);
    while (n--) *vp++ = 0;
}

// Pointer-in-range via uintptr_t (comparing unrelated pointers is UB).
inline bool pointer_in_range(const void* p, const void* base, std::size_t len) noexcept {
    if (!base || len == 0) return false;
    auto pp = reinterpret_cast<std::uintptr_t>(p);
    auto bb = reinterpret_cast<std::uintptr_t>(base);
    return pp >= bb && pp < bb + len;
}

}  // namespace detail
}  // namespace oe

// Abort if cond is false. Couple of compares; catches corruption at the source.
#ifdef ONEDGE_DISABLE_INVARIANT_CHECKS
#  define ONEDGE_CHECK(cond) ((void)0)
#else
#  define ONEDGE_CHECK(cond)                                                 \
      do {                                                                   \
          if (!(cond)) ::oe::detail::invariant_failure(#cond, __FILE__, __LINE__); \
      } while (0)
#endif

#endif  // __cplusplus

#endif  // ONEDGE_CONFIG_H
