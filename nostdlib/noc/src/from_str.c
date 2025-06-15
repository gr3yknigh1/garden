#include "noc/numeric/from_str.h"

#include "noc/macros.h"
#include "noc/types.h"

// TODO: Handle empty string.
bool
noc_i32_from_str(Str8Z str, Int32S *out) {
    if (!str || str[0] == '\0' || !out) {
        return false;
    }

    Int32S num = 0;
    Int32S i = 0;
    bool isnegative = false;
    if (str[i] == '-') {
        isnegative = true;
        i++;
    }
    while (str[i] && (str[i] >= '0' && str[i] <= '9')) {
        num = num * 10 + (str[i] - '0');
        i++;
    }
    if (isnegative)
        num = -1 * num;

    *out = num;
    return true;
}
