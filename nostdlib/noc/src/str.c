#include "noc/str.h"

#include "noc/noc.h"

#include <stdarg.h> // va_list va_start va_end va_arg
#include <stdlib.h> // calloc

static void insert_and_shift(Char8 *str, Str8Z src, long src_index, long str_index);

#define _HANDLE_LENGTH_MODIFIER(LENGTH_MOD, HANDLE_16_EXPR, HANDLE_32_EXPR, HANDLE_64_EXPR)               \
    do {                                                                       \
        switch ((LENGTH_MOD)) {                                              \
        case LENGTH_MODIFIER_16:                                               \
            HANDLE_16_EXPR;                                                   \
            break;                                                             \
        case LENGTH_MODIFIER_32:                                               \
            HANDLE_32_EXPR;                                                   \
            break;                                                             \
        case LENGTH_MODIFIER_64:                                               \
            HANDLE_64_EXPR;                                                   \
            break;                                                             \
        case LENGTH_MODIFIER_128:                                              \
            break;                                                             \
        }                                                                      \
    } while (0)


typedef enum Justification_Type {
    JUSTIFICATION_LEFT,
    JUSTIFICATION_RIGHT,
} Justification_Type;

typedef enum Sign_Precede {
    SIGN_PRECEDE_NONE,
    SIGN_PRECEDE_SPACE,
    SIGN_PRECEDE_FORCE,
} Sign_Precede;

typedef enum Length_Modifier {
    LENGTH_MODIFIER_16,
    LENGTH_MODIFIER_32,
    LENGTH_MODIFIER_64,
    LENGTH_MODIFIER_128,
} Length_Modifier;

typedef struct Format_Context {
    Justification_Type justification_type;
    Int64S justification_value;
    Sign_Precede sign_precede;
    Length_Modifier length_modifier;
    Int32U precision;
    const Char8 **format_read_ptr;
    Char8 curchar;
} Format_Context;

static Int8U fmt_sign_precede_charcount(const Format_Context *ctx, bool is_positive);
static bool fmt_is_justification_or_sign_char(Char8 c);
static bool fmt_is_length_modifier_char(Char8 c);
static bool fmt_is_precision_char(Char8 c);
static Int32U fmt_handle_sign_precede(Format_Context *ctx, Byte *buffer,
				bool is_positive);

static Int32U fmt_parse_i32_while_digit(const Char8 *s, Int32S *out_num);
static Int32U fmt_count_while_digit(const Char8 *s);
static Int32U fmt_emmit_char_in_buffer(Byte *write_buffer, Int32U count, Char8 c);

static void sprintf_Int16S (Format_Context *ctx, Char8 **buffer_write_ptr, Int16S value);
static void sprintf_Int32S (Format_Context *ctx, Char8 **buffer_write_ptr, Int32S value);
static void sprintf_Int64S (Format_Context *ctx, Char8 **buffer_write_ptr, Int64S value);
static void sprintf_Int16U (Format_Context *ctx, Char8 **buffer_write_ptr, Int16U value);
static void sprintf_Int32U (Format_Context *ctx, Char8 **buffer_write_ptr, Int32U value);
static void sprintf_Int64U (Format_Context *ctx, Char8 **buffer_write_ptr, Int64U value);
static void sprintf_Float32(Format_Context *ctx, Char8 **buffer_write_ptr, Float32 value);
static void sprintf_Float64(Format_Context *ctx, Char8 **buffer_write_ptr, Float64 value);
static void sprintf_char   (Format_Context *ctx, Char8 **buffer_write_ptr, Char8 value);
static void sprintf_cstr   (Format_Context *ctx, Char8 **buffer_write_ptr, Str8Z value);

/*
 * Sets defaults for formatter contexts
 *
 * :param `format_read_ptr`: Pointer after `%` char
 * */
static Format_Context fmt_context_init(const Char8 **format_read_ptr);

static void fmt_context_advance(Format_Context *ctx);
static void fmt_context_advance_for(Format_Context *ctx, Int64U count);

/*
 * Parsing formatting context. Should set `format_read_ptr` to conversion
 * specifier.
 * */
static void fmt_context_parse(Format_Context *ctx);

SizeU
noc_str8z_format(Char8 *buffer, const Str8Z format, ...)
{
    const Char8 *format_read_ptr = format;
    Char8 *buffer_write_ptr = buffer;

    va_list args;
    va_start(args, format);

    while (*format_read_ptr != 0) {
        if (*format_read_ptr == '%') {
            ++format_read_ptr;

            Format_Context ctx = fmt_context_init(&format_read_ptr);
            fmt_context_parse(&ctx);

            Char8 convertion_specifier = *format_read_ptr;

            // TODO: Replace with switch-statement
            if (convertion_specifier == 'i' || convertion_specifier == 'd') {
                _HANDLE_LENGTH_MODIFIER(
                    ctx.length_modifier,
                    sprintf_Int16S(&ctx, &buffer_write_ptr, va_arg(args, int)),
                    sprintf_Int32S(&ctx, &buffer_write_ptr, va_arg(args, int)),
                    sprintf_Int64S(&ctx, &buffer_write_ptr, va_arg(args, long)));
            } else if (convertion_specifier == 'c') {
                // FIXME: Handle widechar
                sprintf_char(&ctx, &buffer_write_ptr, va_arg(args, int));
            } else if (convertion_specifier == 'f') {
                // FIXME: Use actual impls for `f32`. Remove this hack
                _HANDLE_LENGTH_MODIFIER(
                    ctx.length_modifier, NOC_NOP,
                    sprintf_Float64(&ctx, &buffer_write_ptr, va_arg(args, double)),
                    sprintf_Float64(&ctx, &buffer_write_ptr,
                                 va_arg(args, double)));
            } else if (convertion_specifier == 's') {
                sprintf_cstr(&ctx, &buffer_write_ptr, va_arg(args, char *));
            } else if (convertion_specifier == 'u') {
                _HANDLE_LENGTH_MODIFIER(
                    ctx.length_modifier,
                    sprintf_Int16U(&ctx, &buffer_write_ptr,
                                 va_arg(args, unsigned int)),
                    sprintf_Int32U(&ctx, &buffer_write_ptr,
                                 va_arg(args, unsigned int)),
                    sprintf_Int64U(&ctx, &buffer_write_ptr,
                                 va_arg(args, unsigned long)));
            } else if (convertion_specifier == 'n') {
                _HANDLE_LENGTH_MODIFIER(
                    ctx.length_modifier,
                    *(va_arg(args, short *)) = buffer_write_ptr - buffer,
                    *(va_arg(args, int *)) = buffer_write_ptr - buffer,
                    *(va_arg(args, long *)) = buffer_write_ptr - buffer);
            } else if (convertion_specifier == '%') {
                *buffer_write_ptr++ = '%';
            } else {
                *buffer_write_ptr++ = *format_read_ptr;
            }
        } else {
            *buffer_write_ptr++ = *format_read_ptr;
        }

        ++format_read_ptr;
    }

    va_end(args);

    return buffer_write_ptr - buffer;
}

SizeU
noc_str8z_copy(Char8 *destination, Str8Z source)
{
    const Char8 *source_cursor = source;

    while (*source_cursor != 0) {
	SizeU index = source_cursor - source;
	destination[index] = source[index];

	++source_cursor;
    }

    SizeU result = source_cursor - source;
    return result;
}

SizeU
noc_str8z_length(const Str8Z s)
{
    const Char8 *cursor = s;

    while (*cursor != '\0') {
	++cursor;
    }

    SizeU result = cursor - s;
    return result;
}

bool
noc_str8z_is_equals(const Str8Z a, const Str8Z b)
{
    const Char8 *a_cursor = a;
    const Char8 *b_cursor = b;
   
    while (a_cursor != NULL && *a_cursor != 0 && b_cursor != NULL && *b_cursor != 0) {
        if (*a_cursor != *b_cursor) {
            return false;
        }

	a_cursor++;
	b_cursor++;
    }

    if ((*a_cursor == 0 && *b_cursor != 0) || (*a_cursor != 0 && *b_cursor != 0)) {
        return false;
    }

    return true;
}

void *
noc_str8z_insert(Str8Z dst, const Str8Z src, SizeU at)
{

    if (dst == NULL || src == NULL) {
        return NULL;
    }

    SizeU dst_len = noc_str8z_length(dst);

    if (at <= dst_len) {
        return NULL;
    }

    void *res = NULL;
    SizeU src_len = noc_str8z_length(src);

    // FIXME: Remove explicit calloc
    if ((res = calloc(dst_len + src_len + 1, sizeof(char))) != NULL) {
        return NULL;
    }

    SizeU insert_index = at;
    noc_memory_copy(res, (char *)dst, dst_len + 1);

    for (SizeU i = 0; i < src_len; ++i, ++insert_index) {
        insert_and_shift(res, src, insert_index, i);
    }

    return res;
}

void
noc_str8z_to_upper(Char8 *s)
{
    while (*s++ != '\0') {
        if (NOC_IS_LOWER(*s)) {
            *s += 'A' - 'a';
        }
    }
}

void
noc_str8z_to_lower(Char8 *s)
{
    while (*s++ != '\0') {
        if (NOC_IS_UPPER(*s)) {
            *s -= 'A' - 'a';
        }
    }
}

///
/// TODO: Replace src with char
///
static void
insert_and_shift(Char8 *str, Str8Z src, long src_index, long str_index)
{
    SizeU src_length = noc_str8z_length(str);
    for (long i = src_length; i >= src_index; --i) {
        str[i + 1] = str[i];
    }
    str[src_index] = src[str_index];
}

NOC_Str8_View
noc_str8_view_make(Str8Z data)
{
    NOC_Str8_View sv;
    sv.data = data;
    sv.length = noc_str8z_length(data);

    return sv;
}

NOC_Str8_View
noc_str8_view_make2(Str8Z data, Int64U length)
{
    NOC_Str8_View sv;
    sv.data = data;
    sv.length = length;  // TODO(gr3yknigh1): Check for validity? [2025/05/31]
    return sv;
}

NOC_Str8_View
noc_str8_view_make_capture(Char8 *begin, Char8 until)
{
    NOC_Str8_View sv;

    sv.data = begin;

    while (*begin != until && begin != 0) {
        begin++;
    }

    sv.length = begin - sv.data;

    return sv;
}

bool
noc_str8_view_copy_to_str8z(NOC_Str8_View sv, Char8 *out, Int64U out_limit)
{
    if (sv.length + 1 > out_limit) {
        return false;
    }

    noc_memory_copy((void *)out, sv.data, sv.length);
    out[sv.length] = 0;

    return true;
}


bool
noc_str8_view_is_equals(NOC_Str8_View a, NOC_Str8_View b)
{
    if (a.length != b.length) {
        return false;
    }

    // TODO(gr3yknigh1): Do vectorization [2025/01/03]
    for (Int64U i = 0; i < a.length; ++i) {
        if (a.data[i] != b.data[i]) {
            return false;
        }
    }

    return true;
}


static Format_Context
fmt_context_init(const Char8 **format_read_ptr)
{
    return (Format_Context) {
        .justification_type = JUSTIFICATION_RIGHT,
        .justification_value = 0,
        .sign_precede = SIGN_PRECEDE_NONE,
        .length_modifier = LENGTH_MODIFIER_32,
        .precision = 6,
        .format_read_ptr = format_read_ptr,
        .curchar = **format_read_ptr,
    };
}

static void
fmt_context_parse(Format_Context *ctx)
{
    while (fmt_is_justification_or_sign_char(ctx->curchar)) {
        switch (ctx->curchar) {
        case '-':
            ctx->justification_type = JUSTIFICATION_LEFT;
            break;
        case '+':
            ctx->sign_precede = SIGN_PRECEDE_FORCE;
            break;
        case ' ':
            if (ctx->sign_precede != SIGN_PRECEDE_FORCE) {
                ctx->sign_precede = SIGN_PRECEDE_SPACE;
            }
            break;
        }
        fmt_context_advance(ctx);
    }

    if (NOC_IS_DIGIT(ctx->curchar)) {
        fmt_context_advance_for(ctx, fmt_parse_i32_while_digit(
                                     *ctx->format_read_ptr,
                                     (Int32S *)(&ctx->justification_value)));
        //                           ^^^^^^^
        // FIXME: Remove this pointer conversion. Replace i32 version with u32
    }

    if (fmt_is_precision_char(ctx->curchar)) {
        fmt_context_advance(ctx);
        fmt_context_advance_for(
            ctx, fmt_parse_i32_while_digit(*ctx->format_read_ptr,
                                            (Int32S *)(&ctx->precision)));
        //                                  ^^^^^^^
        // FIXME: Remove this pointer conversion. Replace i32 version with u32
    }

    if (fmt_is_length_modifier_char(ctx->curchar)) {
        // FIXME: We currently only supporting `h` and `l` length modifiers
        switch (ctx->curchar) {
        case 'l':
            ctx->length_modifier = LENGTH_MODIFIER_64;
            break;
        case 'h':
            ctx->length_modifier = LENGTH_MODIFIER_16;
            break;
        }
        fmt_context_advance(ctx);
    }
}

static void
fmt_context_advance(Format_Context *ctx) {
    if (**ctx->format_read_ptr != '\0') {
        (*ctx->format_read_ptr)++;
        ctx->curchar = **ctx->format_read_ptr;
    }
}

static void
fmt_context_advance_for(Format_Context *ctx, Int64U count) {
    while (count-- > 0) {
        fmt_context_advance(ctx);
    }
}

#define SPRINTF_INT(TYPE)                                                   \
    static void sprintf_##TYPE(Format_Context *ctx, Char8 **buffer_write_ptr,  \
                           TYPE value) {                                     \
        if (ctx->justification_value != 0) {                                   \
            Int32S sign_charcount = fmt_sign_precede_charcount(ctx, value > 0);  \
            Int32S value_charcount = NOC_COUNTDIGITS(value);                      \
            ctx->justification_value = NOC_MAX(                                    \
                ctx->justification_value - value_charcount - sign_charcount,   \
                0);                                                            \
                                                                               \
            switch (ctx->justification_type) {                                 \
            case JUSTIFICATION_LEFT:                                           \
                *buffer_write_ptr += fmt_handle_sign_precede(                 \
                    ctx, *buffer_write_ptr, value > 0);                        \
                *buffer_write_ptr += NOC_TO_STR(value, *buffer_write_ptr);     \
                *buffer_write_ptr += fmt_emmit_char_in_buffer(                \
                    *buffer_write_ptr, ctx->justification_value, ' ');         \
                break;                                                         \
            case JUSTIFICATION_RIGHT:                                          \
                *buffer_write_ptr += fmt_emmit_char_in_buffer(                \
                    *buffer_write_ptr, ctx->justification_value, ' ');         \
                *buffer_write_ptr += fmt_handle_sign_precede(                 \
                    ctx, *buffer_write_ptr, value > 0);                        \
                *buffer_write_ptr += NOC_TO_STR(value, *buffer_write_ptr);     \
                break;                                                         \
            }                                                                  \
        } else {                                                               \
            *buffer_write_ptr +=                                               \
                fmt_handle_sign_precede(ctx, *buffer_write_ptr, value > 0);   \
            *buffer_write_ptr += NOC_TO_STR(value, *buffer_write_ptr);         \
        }                                                                      \
    }

// FIXME: Checkout values: 1232.3 and replace f64 with f32 version
#define SPRINTF_FLT(TYPE)                                                   \
    static void sprintf_##TYPE(Format_Context *ctx, Char8 **buffer_write_ptr,  \
                           TYPE value) {                                     \
        if (ctx->justification_value != 0) {                                   \
            Int32S sign_charcount = fmt_sign_precede_charcount(ctx, value > 0); \
            Int32S value_charcount = NOC_FLT_CHARCOUNT(value, ctx->precision); \
            ctx->justification_value = NOC_MAX(                                    \
                ctx->justification_value - value_charcount - sign_charcount,   \
                0);                                                            \
                                                                               \
            switch (ctx->justification_type) {                                 \
            case JUSTIFICATION_LEFT:                                           \
                *buffer_write_ptr += fmt_handle_sign_precede(                 \
                    ctx, *buffer_write_ptr, value > 0);                        \
                *buffer_write_ptr +=                                           \
                    NOC_TO_STR(value, *buffer_write_ptr, ctx->precision);      \
                *buffer_write_ptr += fmt_emmit_char_in_buffer(                \
                    *buffer_write_ptr, ctx->justification_value, ' ');         \
                break;                                                         \
            case JUSTIFICATION_RIGHT:                                          \
                *buffer_write_ptr += fmt_emmit_char_in_buffer(                \
                    *buffer_write_ptr, ctx->justification_value, ' ');         \
                *buffer_write_ptr += fmt_handle_sign_precede(                 \
                    ctx, *buffer_write_ptr, value > 0);                        \
                *buffer_write_ptr +=                                           \
                    NOC_TO_STR(value, *buffer_write_ptr, ctx->precision);      \
                break;                                                         \
            }                                                                  \
        } else {                                                               \
            *buffer_write_ptr +=                                               \
                fmt_handle_sign_precede(ctx, *buffer_write_ptr, value > 0);   \
            *buffer_write_ptr +=                                               \
                NOC_TO_STR(value, *buffer_write_ptr, ctx->precision);          \
        }                                                                      \
    }

SPRINTF_INT(Int16S)
SPRINTF_INT(Int32S)
SPRINTF_INT(Int64S)

SPRINTF_INT(Int16U)
SPRINTF_INT(Int32U)
SPRINTF_INT(Int64U)

// FIXME: Replace `Float64` on `float` usage in sprintf with this
// static _SPRINTF_FLT(Float32)
SPRINTF_FLT(Float64)

static void
sprintf_char(Format_Context *ctx, Char8 **buffer_write_ptr, Char8 value) {
    if (ctx->justification_value != 0) {
        Int32S sign_charcount = fmt_sign_precede_charcount(ctx, value > 0);
        Int32S value_charcount = 1;
        ctx->justification_value =
            NOC_MAX(ctx->justification_value - value_charcount - sign_charcount, 0);

        switch (ctx->justification_type) {
        case JUSTIFICATION_LEFT:
            *buffer_write_ptr +=
                fmt_handle_sign_precede(ctx, *buffer_write_ptr, value > 0);
            *(*buffer_write_ptr)++ += value;
            *buffer_write_ptr += fmt_emmit_char_in_buffer(
                *buffer_write_ptr, ctx->justification_value, ' ');
            break;
        case JUSTIFICATION_RIGHT:
            *buffer_write_ptr += fmt_emmit_char_in_buffer(
                *buffer_write_ptr, ctx->justification_value, ' ');
            *buffer_write_ptr +=
                fmt_handle_sign_precede(ctx, *buffer_write_ptr, value > 0);
            *(*buffer_write_ptr)++ += value;
            break;
        }
    } else {
        *(*buffer_write_ptr)++ += value;
    }
}

static void
sprintf_cstr(Format_Context *ctx, Char8 **buffer_write_ptr, const Str8Z value)
{
    Int32U value_charcount = noc_str8z_length(value);

    if (ctx->justification_value != 0) {
        ctx->justification_value =
            NOC_MAX(ctx->justification_value - value_charcount, 0);

        switch (ctx->justification_type) {
        case JUSTIFICATION_LEFT:
            *buffer_write_ptr += noc_str8z_copy(*buffer_write_ptr, value);
            *buffer_write_ptr += fmt_emmit_char_in_buffer(
                *buffer_write_ptr, ctx->justification_value, ' ');
            break;
        case JUSTIFICATION_RIGHT:
            *buffer_write_ptr += fmt_emmit_char_in_buffer(
                *buffer_write_ptr, ctx->justification_value, ' ');
            *buffer_write_ptr += noc_str8z_copy(*buffer_write_ptr, value);
            break;
        }
    } else {
        *buffer_write_ptr += noc_str8z_copy(*buffer_write_ptr, value);
    }
}

static Int8U
fmt_sign_precede_charcount(const Format_Context *ctx, bool is_positive)
{
    Int8U charcount = 0;

    if ((ctx->sign_precede == SIGN_PRECEDE_FORCE && is_positive) ||
        (ctx->sign_precede == SIGN_PRECEDE_SPACE)) {
        charcount = 1;
    }

    return charcount;
}

static bool
fmt_is_justification_or_sign_char(Char8 c)
{
    return c == '-' || c == ' ' || c == '+';
}

static bool
fmt_is_length_modifier_char(Char8 c)
{
    return c == 'l' || c == 'h';
}

static bool
fmt_is_precision_char(Char8 c)
{
    return c == '.';
}

static Int32U
fmt_handle_sign_precede(Format_Context *ctx, Byte *buffer, bool is_positive)
{
    Byte *buffer_write_ptr = buffer;

    if (ctx->sign_precede == SIGN_PRECEDE_FORCE && is_positive) {
        *buffer_write_ptr++ = '+';
        ctx->justification_value--;
    }

    if (ctx->sign_precede == SIGN_PRECEDE_SPACE && is_positive) {
        *buffer_write_ptr++ = ' ';
        ctx->justification_value--;
    }

    return buffer_write_ptr - buffer;
}

static Int32U
fmt_count_while_digit(const Char8 *s) {
    Int32U count = 0;
    while (NOC_IS_DIGIT(*s)) {
        ++count;
        ++s;
    }
    return count;
}

static Int32U
fmt_parse_i32_while_digit(const Char8 *s, Int32S *out_num)
{
    Int32U numlen = fmt_count_while_digit(s);

    Char8 *numstr = (Char8 *)noc_allocate(numlen + 1); // @perf Remove this malloc
    numstr[numlen] = '\0';

    noc_memory_copy(numstr, s, numlen);

    if (out_num != NULL) {
        // TODO: Replace in the future with generic version.
        // TODO: Handle error from convertion.
        noc_i32_from_str(numstr, out_num);
    }

    noc_free(numstr);

    return numlen;
}

static Int32U
fmt_emmit_char_in_buffer(Byte *write_buffer, Int32U count, Char8 c)
{
    for (Int32U i = 0; i < count; ++i) {
        *write_buffer++ = c;
    }
    return count;
}

#if defined(NOC_LIBC_WRAPPERS)

NOC_DEFINE void
noc_print_hex_dump(void *buffer, SizeU buffer_size)
{
    noc_print_hex_dump_ex(buffer, buffer_size, stdout);
}


NOC_DEFINE void
noc_print_hex_dump_ex(void *buffer, SizeU buffer_size, FILE *output)
{
    for (SizeU i = 0; i < buffer_size; i += 16) {
        fprintf(output, "%06llx: ", i);

        for (SizeU j = 0; j < 16; j++) {
            if (i + j < buffer_size) {
                fprintf(output, "%02x ", ((Byte *)buffer)[i + j]);
            } else {
                fprintf(output, "   ");
            }
        }

        printf(" ");
        for (SizeU j = 0; j < 16; j++) {
            if (i + j < buffer_size) {
                fprintf(output, "%c", isprint(((Byte *)buffer)[i + j]) ? ((Byte *)buffer)[i + j] : '.');
            }
        }

        fprintf(output, "\n");
    }

}

#endif // NOC_LIBC_WRAPPERS
