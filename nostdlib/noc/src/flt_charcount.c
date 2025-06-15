#include "noc/numeric/flt_charcount.h"

#include "noc/math/mod.h"
#include "noc/numeric/countdigits.h"

#define _NOC_FLT_CHARCOUNT_IMPL(__TYPE)                                        \
    _NOC_FLT_CHARCOUNT_DEF(__TYPE) {                                           \
        Int16U count = 0;                                                      \
        __TYPE ipart = 0;                                                      \
        NOC_MOD(number, &ipart);                                               \
        count += NOC_COUNTDIGITS((Int64S)ipart);                               \
        if (precision > 0) {                                                   \
            count += 1;                                                        \
            count += precision;                                                \
        }                                                                      \
        return count;                                                          \
    }

_NOC_FLT_CHARCOUNT_IMPL(Float32)
_NOC_FLT_CHARCOUNT_IMPL(Float64)
