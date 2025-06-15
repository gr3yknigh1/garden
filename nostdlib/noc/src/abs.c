#include "noc/numeric/abs.h"

#define _NOC_ABS_IMPL(__TYPE)                                                  \
    _NOC_ABS_DEF(__TYPE) { return x >= 0 ? x : x * -1; }

_NOC_ABS_IMPL(Int16S)
_NOC_ABS_IMPL(Int32S)
_NOC_ABS_IMPL(Int64S)
_NOC_ABS_IMPL(Float32)
_NOC_ABS_IMPL(Float64)
