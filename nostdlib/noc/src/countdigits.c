#include "noc/numeric/countdigits.h"

#define _NOC_COUNTDIGITS_IMPL(__TYPE)                                          \
    _NOC_COUNTDIGITS_DEF(__TYPE) {                                             \
        if (n == 0)                                                            \
            return 1;                                                          \
        Int64U count = 0;                                                         \
        while (n != 0) {                                                       \
            n = n / 10;                                                        \
            ++count;                                                           \
        }                                                                      \
        return count;                                                          \
    }

_NOC_COUNTDIGITS_IMPL(Int16S)
_NOC_COUNTDIGITS_IMPL(Int32S)
_NOC_COUNTDIGITS_IMPL(Int64S)
_NOC_COUNTDIGITS_IMPL(Int16U)
_NOC_COUNTDIGITS_IMPL(Int32U)
_NOC_COUNTDIGITS_IMPL(Int64U)
