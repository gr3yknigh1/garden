#ifndef NOC_NUMERIC_FROM_STR_H_INCLUDED
#define NOC_NUMERIC_FROM_STR_H_INCLUDED

#include <noc/macros.h>
#include <noc/types.h>

#define NOC_FROM_STR(__X)                                                      \
    _Generic((__X), Int16S                                                        \
             : noc_i32_from_str, Int32S                                           \
             : noc_i32_from_str, Int32S                                           \
             : noc_i32_from_str, default                                       \
             : noc_i32_from_str)(__X)
// FIXME: Add other generics.

NOC_DEFINE bool noc_i32_from_str(Str8Z str, Int32S *out);

#endif // NOC_NUMERIC_FROM_STR_H_INCLUDED
