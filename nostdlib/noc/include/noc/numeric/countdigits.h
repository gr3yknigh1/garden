#ifndef NOSTDLIB_NUMERIC_COUNTDIGITS_H_
#define NOSTDLIB_NUMERIC_COUNTDIGITS_H_

#include "noc/macros.h"
#include "noc/types.h"

#define NOC_COUNTDIGITS(__X)                                                   \
    _Generic((__X), Int16S                                                        \
             : _NOC_COUNTDIGITS_NAME(Int16S), Int32S                                 \
             : _NOC_COUNTDIGITS_NAME(Int32S), Int64S                                 \
             : _NOC_COUNTDIGITS_NAME(Int64S), Int16U                                 \
             : _NOC_COUNTDIGITS_NAME(Int16U), Int32U                                 \
             : _NOC_COUNTDIGITS_NAME(Int32U), Int64U                                 \
             : _NOC_COUNTDIGITS_NAME(Int64U), default                             \
             : _NOC_COUNTDIGITS_NAME(Int32S))(__X)

#define _NOC_COUNTDIGITS_NAME(__TYPE) noc_##__TYPE##_countdigits
#define _NOC_COUNTDIGITS_DEF(__TYPE)                                           \
    NOC_DEFINE Int64U _NOC_COUNTDIGITS_NAME(__TYPE)(__TYPE n)

_NOC_COUNTDIGITS_DEF(Int16S);
_NOC_COUNTDIGITS_DEF(Int32S);
_NOC_COUNTDIGITS_DEF(Int64S);
_NOC_COUNTDIGITS_DEF(Int16U);
_NOC_COUNTDIGITS_DEF(Int32U);
_NOC_COUNTDIGITS_DEF(Int64U);

#endif // NOSTDLIB_NUMERIC_COUNTDIGITS_H_
