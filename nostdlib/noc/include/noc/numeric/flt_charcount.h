#ifndef NOSTDLIB_NUMERIC_FLT_CHARCOUNT_H_
#define NOSTDLIB_NUMERIC_FLT_CHARCOUNT_H_

#include "noc/macros.h"
#include "noc/types.h"

/*
 * Returns count of characters which may occupy floating-point number
 * */
#define NOC_FLT_CHARCOUNT(__X, __PRECISION)                                    \
    _Generic((__X), Float32                                                    \
             : _NOC_FLT_CHARCOUNT_NAME(Float32), Float64                       \
             : _NOC_FLT_CHARCOUNT_NAME(Float64), default                       \
             : _NOC_FLT_CHARCOUNT_NAME(Float32))(__X, __PRECISION)

#define _NOC_FLT_CHARCOUNT_NAME(__TYPE) noc_##__TYPE##_charcount

#define _NOC_FLT_CHARCOUNT_DEF(__TYPE)                                         \
    NOC_DEFINE Int16U _NOC_FLT_CHARCOUNT_NAME(__TYPE)(__TYPE number, Int8U precision)

_NOC_FLT_CHARCOUNT_DEF(Float32);
_NOC_FLT_CHARCOUNT_DEF(Float64);

#endif // NOSTDLIB_NUMERIC_FLT_CHARCOUNT_H_
