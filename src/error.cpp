#include "onedge/error.h"

#include <cstdio>
#include <cstdlib>

namespace oe {

namespace detail {

void invariant_failure(const char* expr, const char* file, int line) noexcept {
    // abort, not exit -- want a core dump for post-mortem.
    std::fprintf(stderr,
                 "onedge: internal invariant violated: %s\n"
                 "  at %s:%d\n"
                 "  aborting to prevent operating on corrupt state\n",
                 expr ? expr : "(null)",
                 file ? file : "(unknown)",
                 line);
    std::fflush(stderr);
    std::abort();
}

}  // namespace detail

const char* error_message(error_code code) noexcept {
    switch (code) {
        case error_code::ok:                  return "ok";
        case error_code::out_of_memory:       return "out of memory";
        case error_code::invalid_argument:    return "invalid argument";
        case error_code::out_of_range:        return "index or range out of bounds";
        case error_code::overflow:            return "integer overflow in size calculation";
        case error_code::truncated:           return "operation would truncate input";
        case error_code::not_null_terminated: return "input is not null-terminated";
        case error_code::capacity_exceeded:   return "operation would exceed configured capacity cap";
        case error_code::null_pointer:        return "null pointer passed where one was not allowed";
        case error_code::unsupported:         return "operation is not supported";
        case error_code::parse_error:         return "input could not be parsed";
    }
    return "unknown error";
}

}  // namespace oe
