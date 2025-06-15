#ifndef NOC_NUMERIC_ABS_H_INCLUDED
#define NOC_NUMERIC_ABS_H_INCLUDED

#include "noc/types.h"

#define NOC_ABS(__X)                                                           \
    _Generic((__X), Int16S                                                     \
             : _NOC_ABS_NAME(Int16S), Int32S                                   \
             : _NOC_ABS_NAME(Int32S), Int64S                                   \
             : _NOC_ABS_NAME(Int64S), Float32                                  \
             : _NOC_ABS_NAME(Float32), Float64                                 \
             : _NOC_ABS_NAME(Float32), default                                 \
             : _NOC_ABS_NAME(Int32S))(__X)

#define _NOC_ABS_NAME(__TYPE) noc_##__TYPE##_abs
#define _NOC_ABS_DEF(__TYPE) __TYPE _NOC_ABS_NAME(__TYPE)(__TYPE x)

extern _NOC_ABS_DEF(Int16S);
extern _NOC_ABS_DEF(Int32S);
extern _NOC_ABS_DEF(Int64S);
extern _NOC_ABS_DEF(Float32);
extern _NOC_ABS_DEF(Float64);

#endif // NOC_NUMERIC_ABS_H_INCLUDED
