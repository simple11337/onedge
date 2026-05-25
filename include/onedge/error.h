#ifndef ONEDGE_ERROR_H
#define ONEDGE_ERROR_H

#include "onedge/onedge_config.h"

namespace oe {

// Numeric values are ABI; mirrored in oe_status. New codes go at the
// end, numbers are never reused.
enum class error_code : int {
    ok                  = 0,
    out_of_memory       = 1,
    invalid_argument    = 2,
    out_of_range        = 3,
    overflow            = 4,
    truncated           = 5,
    not_null_terminated = 6,
    capacity_exceeded   = 7,
    null_pointer        = 8,
    unsupported         = 9,
    parse_error         = 10,
};

// Stable NUL-terminated literal. Never null.
ONEDGE_API const char* error_message(error_code code) noexcept;

}  // namespace oe

#endif  // ONEDGE_ERROR_H
