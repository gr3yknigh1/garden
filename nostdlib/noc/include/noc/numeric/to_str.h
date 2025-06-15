#ifndef NOSTDLIB_NUMERIC_TO_STR_H_
#define NOSTDLIB_NUMERIC_TO_STR_H_

#include <noc/macros.h>
#include <noc/types.h>

// noc_to_str :: (<number>, <buffer>, [precision])
#define NOC_TO_STR(__X, ...)						\
    _Generic((__X), Int16S						\
             : _NOC_TO_STR_NAME(Int16S), Int32S				\
             : _NOC_TO_STR_NAME(Int32S), Int64S				\
             : _NOC_TO_STR_NAME(Int64S), Int16U				\
             : _NOC_TO_STR_NAME(Int16U), Int32U				\
             : _NOC_TO_STR_NAME(Int32U), Int64U				\
             : _NOC_TO_STR_NAME(Int64U), Float32			\
             : _NOC_TO_STR_NAME(Float32), Float64			\
             : _NOC_TO_STR_NAME(Float64), default			\
             : _NOC_TO_STR_NAME(Int32S))(__X, __VA_ARGS__)

#define _NOC_TO_STR_NAME(__TYPE) noc_##__TYPE##_to_str

#define _NOC_INT_TO_STR_DEF(__TYPE)                                            \
    NOC_DEFINE SizeU _NOC_TO_STR_NAME(__TYPE)(__TYPE number, char *buffer)

#define _NOC_FLT_TO_STR_DEF(__TYPE)                                            \
    NOC_DEFINE SizeU _NOC_TO_STR_NAME(__TYPE)(__TYPE number, char *buffer,        \
                                           Int8U precision)

_NOC_INT_TO_STR_DEF(Int16S);
_NOC_INT_TO_STR_DEF(Int32S);
_NOC_INT_TO_STR_DEF(Int64S);
_NOC_INT_TO_STR_DEF(Int16U);
_NOC_INT_TO_STR_DEF(Int32U);
_NOC_INT_TO_STR_DEF(Int64U);
_NOC_FLT_TO_STR_DEF(Float32);
_NOC_FLT_TO_STR_DEF(Float64);

#endif // NOSTDLIB_NUMERIC_TO_STR_H_
