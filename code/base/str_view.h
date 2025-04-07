//!
//! FILE          code\base\str_view.h
//!
//! AUTHORS
//!               Ilya Akkuzin <gr3yknigh1@gmail.com>
//!
//! NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//!
#pragma once

#include "base/str.h"

struct Str8_View {
    const char *data;
    size_t length;

    constexpr inline bool empty(void) const noexcept { return this->length == 0; }

    constexpr inline Str8_View() noexcept : data(nullptr), length(0) {}
    constexpr inline Str8_View(const char *data_) noexcept : data(data_), length(str8_get_length(data_)) {}
    constexpr inline Str8_View(const char *data_, size_t length_) noexcept : data(data_), length(length_) {}

    constexpr inline Str8_View(const Str8 &str) : Str8_View(str.data, str.length) {}
};


inline Str8_View
str8_view_capture_until(const char **cursor, char until)
{
    Str8_View sv;

    sv.data = *cursor;

    while (**cursor != until) {
        (*cursor)++;
    }

    sv.length = *cursor - sv.data;

    return sv;
}

inline bool
str8_view_copy_to_nullterminated(Str8_View sv, char *out_buffer, size_t out_buffer_size)
{
    if (sv.length + 1 > out_buffer_size) {
        return false;
    }

    memcpy((void *)out_buffer, sv.data, sv.length);
    out_buffer[sv.length] = 0;
    return true;
}

constexpr bool
str8_view_is_equals(Str8_View a, Str8_View b)
{
    if (a.length != b.length) {
        return false;
    }

    // TODO(gr3yknigh1): Do vectorization [2025/01/03]
    for (size_t i = 0; i < a.length; ++i) {
        if (a.data[i] != b.data[i]) {
            return false;
        }
    }

    return true;
}

constexpr bool
str8_view_is_equals(Str8_View a, const char *str)
{
    Str8_View b(str);
    return str8_view_is_equals(a, b);
}

constexpr size_t
str16_get_length(const wchar_t *s) noexcept
{
    size_t result = 0;

    while (s[result] != 0) {
        result++;
    }

    return result;
}

struct Str16_View {
    const wchar_t *data;
    size_t length;

    constexpr Str16_View() noexcept : data(nullptr), length(0) {}
    constexpr Str16_View(const wchar_t *data_) noexcept : data(data_), length(str16_get_length(data_)) {}
    constexpr Str16_View(const wchar_t *data_, size_t length_) noexcept : data(data_), length(length_) {}
};

inline bool
str16_view_copy_to_nullterminated(Str16_View view, wchar_t *out_buffer, size_t out_buffer_size) noexcept
{
    size_t required_buffer_size = (view.length + 1) * sizeof(*view.data);

    if (required_buffer_size > out_buffer_size) {
        return false;
    }

    memcpy((void *)out_buffer, view.data, view.length * sizeof(*view.data));
    out_buffer[view.length] = 0;
    return true;
}

inline bool
str16_view_endswith(Str16_View view, Str16_View end) noexcept
{
    if (view.length < end.length) {
        return false;
    }

    for (size_t end_index = end.length - 1, view_index = view.length - 1; end_index > 0; --end_index, --view_index) {
        if (end.data[end_index] != view.data[view_index]) {
            return false;
        }
    }

    return true;
}

inline bool
str16_view_endswith(Str16_View view, Str8_View end) noexcept
{
    if (view.length < end.length) {
        return false;
    }

    for (size_t end_index = end.length - 1, view_index = view.length - 1; end_index > 0; --end_index, --view_index) {

        const char *c16 = reinterpret_cast<const char *>(view.data + view_index);

        if (end.data[end_index] != c16[0]) {
            return false;
        }

        if (c16[1] != 0) {
            return false;
        }
    }

    return true;
}

constexpr bool
str16_view_is_equals(const Str16_View a, const Str16_View b) noexcept
{
    if (a.length != b.length) {
        return false;
    }

    // TODO(gr3yknigh1): Do vectorization [2025/01/03]
    for (size_t i = 0; i < a.length; ++i) {
        if (a.data[i] != b.data[i]) {
            return false;
        }
    }

    return true;
}

constexpr bool
str16_view_is_equals(Str16_View a, const wchar_t *str) noexcept
{
    Str16_View b(str);
    return str16_view_is_equals(a, b);
}

constexpr bool
str16_view_is_equals(const Str16_View a, const Str8_View b) noexcept
{
    if (a.length != b.length) {
        return false;
    }

    // TODO(gr3yknigh1): Do vectorization [2025/01/03]
    for (size_t i = 0; i < a.length; ++i) {
        // TODO(gr3yknigh1): Maybe make c16 be `Byte c16[2]` ? [2025/04/07]
        const Byte *c16 = reinterpret_cast<const Byte *>(a.data + i);

        if (c16[1] != 0) {
            return false;
        }

        Byte c8 = b.data[i];

        if (c16[0] != c8) {
            return false;
        }
    }

    return true;
}

constexpr bool
str16_view_is_equals(const Str16_View a, const char *str) noexcept
{
    Str8_View b(str);
    return str16_view_is_equals(a, b);
}
