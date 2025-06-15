#include "noc/numeric/to_str.h"

#include "noc/math/mod.h"
#include "noc/math/round.h"
#include "noc/numeric/abs.h"
#include "noc/numeric/countdigits.h"

// TODO(gr3yknigh1): Remove ccount and replace with pointer
// arithmetic
#define _NOC_SINT_TO_STR_IMPL(__TYPE)                                          \
    _NOC_INT_TO_STR_DEF(__TYPE) {                                              \
        Int64U ccount = 0;                                                        \
        bool isnegative = number < 0;                                          \
        char *write_ptr =                                                      \
            buffer + NOC_COUNTDIGITS(number) - 1 + (isnegative ? 1 : 0);       \
        number = NOC_ABS(number);                                              \
                                                                               \
        do {                                                                   \
            int digit = number % 10;                                           \
            *write_ptr-- = digit + '0';                                        \
            ccount++;                                                          \
        } while ((number /= 10) > 0);                                          \
                                                                               \
        if (isnegative) {                                                      \
            *write_ptr-- = '-';                                                \
            ccount++;                                                          \
        }                                                                      \
                                                                               \
        return ccount;                                                         \
    }

_NOC_SINT_TO_STR_IMPL(Int16S)
_NOC_SINT_TO_STR_IMPL(Int32S)
_NOC_SINT_TO_STR_IMPL(Int64S)

#undef _NOC_SINT_TO_STR_IMPL

#define _NOC_UINT_TO_STR_IMPL(__TYPE)                                          \
    _NOC_INT_TO_STR_DEF(__TYPE) {                                              \
        Int64U ccount = 0;                                                        \
        char *write_ptr = buffer + NOC_COUNTDIGITS(number) - 1;                \
                                                                               \
        do {                                                                   \
            int digit = number % 10;                                           \
            *write_ptr-- = digit + '0';                                        \
            ccount++;                                                          \
        } while ((number /= 10) > 0);                                          \
                                                                               \
        return ccount;                                                         \
    }

_NOC_UINT_TO_STR_IMPL(Int16U)
_NOC_UINT_TO_STR_IMPL(Int32U)
_NOC_UINT_TO_STR_IMPL(Int64U)

#undef _NOC_UINT_TO_STR_IMPL

#define _NOC_FLT_TO_STR_IMPL(__TYPE)                                           \
    _NOC_FLT_TO_STR_DEF(__TYPE) {                                              \
        char *buffer_write_ptr = buffer;                                       \
                                                                               \
        __TYPE ipart = 0;                                                      \
        __TYPE fpart = NOC_MOD(number, &ipart);                                \
                                                                               \
        buffer_write_ptr += NOC_TO_STR((Int64S)ipart, buffer_write_ptr);          \
                                                                               \
        if (precision > 0) {                                                   \
            *buffer_write_ptr++ = '.';                                         \
                                                                               \
            while (precision-- > 0) {                                          \
                fpart *= 10;                                                   \
                NOC_MOD(fpart, &ipart);                                        \
                                                                               \
                if ((Int64S)ipart == 0 && precision > 0) {                        \
                    *buffer_write_ptr++ = '0';                                 \
                }                                                              \
            }                                                                  \
                                                                               \
            buffer_write_ptr +=                                                \
                NOC_TO_STR(NOC_ABS((Int64S)NOC_ROUND(fpart)), buffer_write_ptr);  \
        }                                                                      \
                                                                               \
        return buffer_write_ptr - buffer;                                      \
    }

_NOC_FLT_TO_STR_IMPL(Float32)
_NOC_FLT_TO_STR_IMPL(Float64)

#undef _NOC_FLT_TO_STR_IMPL
