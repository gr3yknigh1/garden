#ifndef NOC_STR_H_INCLUDED
#define NOC_STR_H_INCLUDED

#include <noc/macros.h>
#include <noc/types.h>

NOC_DEFINE SizeU noc_str8z_format(Char8 *buffer, const Str8Z format, ...);

NOC_DEFINE SizeU noc_str8z_copy(Char8 *destination, Str8Z source);

NOC_DEFINE SizeU noc_str8z_length(Str8Z s);

NOC_DEFINE bool  noc_str8z_is_equals(const Char8 *const a, const Char8 *const b);

NOC_DEFINE void *noc_str8z_insert(Str8Z dst, const Str8Z src, SizeU at);

NOC_DEFINE void  noc_str8z_to_upper(Char8 *s);

NOC_DEFINE void  noc_str8z_to_lower(Char8 *s);


typedef struct NOC_Str8_View {
    Str8Z data;
    Int64U length;
} NOC_Str8_View;

NOC_DEFINE NOC_Str8_View noc_str8_view_make(Str8Z data);
NOC_DEFINE NOC_Str8_View noc_str8_view_make2(Str8Z data, Int64U length);

NOC_DEFINE NOC_Str8_View noc_str8_view_make_capture(Char8 *begin, Char8 until);
NOC_DEFINE bool          noc_str8_view_copy_to_str8z(NOC_Str8_View sv, Char8 *out, Int64U out_limit);

NOC_DEFINE bool          noc_str8_view_is_equals(NOC_Str8_View a, NOC_Str8_View b);


typedef struct NOC_Str16_View {
    const Str16Z data;
    Int64U length;
} NOC_Str16_View;


#if defined(NOC_DETECT_LANGUAGE_CXX)

namespace noxx {

constexpr SizeU
str8z_length(const Str8Z s) noexcept
{
    const Char8 *cursor = s;

    while (*cursor != '\0') {
	++cursor;
    }

    SizeU result = cursor - s;
    return result;
}

} // namespace noxx

#endif

#if defined(NOC_LIBC_WRAPPERS)

#include <stdio.h>

//!
//! @brief Dumps into console hex-view of the memory (formatted).
//!
//! @todo(gr3yknigh1): Add option to dump it all to other place (file for example) [2025/04/25]
//!
NOC_DEFINE void noc_print_hex_dump(void *buffer, Int64U buffer_length);

//!
//! @brief Dumps into console hex-view of the memory (formatted).
//!
NOC_DEFINE void noc_print_hex_dump_ex(void *buffer, Int64U buffer_length, FILE *output);

#endif  // NOC_LIBC_WRAPPERS

#endif // NOC_STR_H_INCLUDED
