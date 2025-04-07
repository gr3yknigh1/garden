//!
//! FILE          code\base\str.h
//!
//! AUTHORS
//!               Ilya Akkuzin <gr3yknigh1@gmail.com>
//!
//! NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//!
#pragma once

constexpr size_t
str8_get_length(const char *s) noexcept
{
    size_t result = 0;

    while (s[result] != 0) {
        result++;
    }

    return result;
}

//!
//! NOTE(gr3yknigh1): Maybe report overflow? [2025/04/06]
//!
constexpr void
str8_copy_to(void *destination, const char *source, size_t source_length, size_t destination_size) noexcept
{
    size_t index = 0;

    while (index < source_length && index < destination_size) {
        static_cast<mm::byte *>(destination)[index] = source[index];
        ++index;
    }
}

//!
//! @brief Owning string type which is implemented using RAII.
//!
class Str8 {
public:
    char *data;
    size_t length;

    constexpr inline Str8(void) noexcept : data(nullptr), length(0) {}

    constexpr explicit
    Str8(const char *data_) noexcept
        : Str8(data_, str8_get_length(data_))
    { }

    constexpr explicit
    Str8(const char *data_, size_t length_) noexcept
        : data(nullptr), length(length_)
    {
        if (data_ && length_) {
            size_t data_buffer_size = this->length + 1;

            void *data_buffer = mm::allocate(data_buffer_size);
            assert(data_buffer);
            mm::zero_memory(data_buffer, data_buffer_size);

            str8_copy_to(data_buffer, data_, this->length, data_buffer_size);

            this->data = static_cast<char *>(data_buffer);
        }
    }

    constexpr explicit
    Str8(const Str8 &other) noexcept
        : Str8(other.data, other.length)
    { }

    constexpr explicit
    Str8(Str8 &&other) noexcept
        : data(std::exchange(other.data, nullptr)), length(std::exchange(other.length, 0))
    { }


    Str8 &
    operator=(const Str8& other) noexcept
    {
        this->destroy();

        return *this = Str8(other);
    }

    Str8 &
    operator=(Str8&& other) noexcept
    {
        this->destroy();

        this->data = std::exchange(other.data, nullptr);
        this->length = std::exchange(other.length, 0);

        return *this;
    }

    ~Str8(void) noexcept
    {
        this->destroy();
    }

    void
    destroy(void) noexcept
    {
        if (this->data) {
            mm::deallocate(static_cast<void *>(this->data));
        }
        this->length = 0;
    }
};
