//!
//! This is shared runtime code.
//!
//! FILE          code\garden_runtime.h
//!
//! AUTHORS
//!               Ilya Akkuzin <gr3yknigh1@gmail.com>
//!
//! NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//!
#pragma once

#include <stdio.h>

#include <source_location>
#include <list>
#include <memory>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/ext/matrix_transform.hpp>

#if !defined(MAKE_FLAG)
    #define MAKE_FLAG(INDEX) (1 << (INDEX))
#endif

#if !defined(HAS_FLAG)
    #define HAS_FLAG(MASK, FLAG) (((MASK) & (FLAG)) == (FLAG))
#endif

#if !defined(LITERAL)
    #if defined(__cplusplus)
        #define LITERAL(X) X
    #else
        #define LITERAL(X) (X)
    #endif
#endif

#if !defined(STATIC_ARRAY_COUNT)
    #define STATIC_ARRAY_COUNT(ARRAY_PTR) (sizeof((ARRAY_PTR)) / sizeof(*(ARRAY_PTR)))
#endif

#if !defined(STRINGIFY_IMPL)
    #define STRINGIFY_IMPL(X) #X
#endif

#if !defined(STRINGIFY)
    #define STRINGIFY(X) STRINGIFY_IMPL(X)
#endif

#if !defined(FOR_LINKED_LIST)
    #define FOR_LINKED_LIST(ITER, LINKED_LIST)                                         \
        for (auto ITER = (LINKED_LIST)->head; ITER != nullptr; ITER = ITER->next)
#endif


#if !defined(EXPAND)
    #define EXPAND(X) X
#endif


#if !defined(__cplusplus)
    // NOTE(gr3yknigh1): Require C11 [2025/04/07]
    #define STATIC_ASSERT(COND, MSG) _Static_assert((COND), MSG)
#else
    #define STATIC_ASSERT(COND, MSG) static_assert((COND), MSG)
#endif

#define EXPECT_TYPE_SIZE(TYPE, SIZE) STATIC_ASSERT(sizeof(TYPE) == (SIZE), "Expected " #TYPE " to be the size of " #SIZE)


/*

    BYTE: An 8-bit unsigned integer value
    WORD: A 16-bit unsigned integer value
    SHORT: A 16-bit signed integer value
    DWORD: A 32-bit unsigned integer value
    LONG: A 32-bit signed integer value
    FIXED: A 32-bit fixed point (16.16) value
    FLOAT: A 32-bit single-precision value
    DOUBLE: A 64-bit double-precision value
    QWORD: A 64-bit unsigned integer value
    LONG64: A 64-bit signed integer value
    BYTE[n]: "n" bytes.
    STRING:
        WORD: string length (number of bytes)
        BYTE[length]: characters (in UTF-8) The '\0' character is not included.
    POINT:
        LONG: X coordinate value
        LONG: Y coordinate value
    SIZE:
        LONG: Width value
        LONG: Height value
    RECT:
        POINT: Origin coordinates
        SIZE: Rectangle size
    PIXEL: One pixel, depending on the image pixel format:
        RGBA: BYTE[4], each pixel have 4 bytes in this order Red, Green, Blue, Alpha.
        Grayscale: BYTE[2], each pixel have 2 bytes in the order Value, Alpha.
        Indexed: BYTE, each pixel uses 1 byte (the index).
    TILE: Tilemaps: Each tile can be a 8-bit (BYTE), 16-bit (WORD), or 32-bit (DWORD) value and there are masks related to the meaning of each bit.
    UUID: A Universally Unique Identifier stored as BYTE[16].

 */

#if !defined(POINTER_SIZE)
    #define POINTER_SIZE 8
#endif

EXPECT_TYPE_SIZE(void *, POINTER_SIZE);

typedef signed char S8;
typedef signed short S16;
typedef signed int S32;

#if defined(_WIN32)
    typedef signed long long S64;
#else
    typedef signed long S64;
#endif

EXPECT_TYPE_SIZE(S8, 1);
EXPECT_TYPE_SIZE(S16, 2);
EXPECT_TYPE_SIZE(S32, 4);
EXPECT_TYPE_SIZE(S64, 8);

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

#if defined(_WIN32)
    typedef unsigned long long U64;
#else
    typedef unsigned long U64;
#endif

EXPECT_TYPE_SIZE(U8, 1);
EXPECT_TYPE_SIZE(U16, 2);
EXPECT_TYPE_SIZE(U32, 4);
EXPECT_TYPE_SIZE(U64, 8);

typedef float  F32;
typedef double F64;

EXPECT_TYPE_SIZE(F32, 4);
EXPECT_TYPE_SIZE(F64, 8);

typedef U8  Byte;
typedef U64 USize;
typedef S64 SSize;

EXPECT_TYPE_SIZE(Byte, 1);
EXPECT_TYPE_SIZE(USize, POINTER_SIZE);
EXPECT_TYPE_SIZE(SSize, POINTER_SIZE);

typedef char     Char8;
EXPECT_TYPE_SIZE(Char8, 1);

typedef wchar_t  Char16;
EXPECT_TYPE_SIZE(Char16, 2);

// NOTE(gr3yknigh1): Explicitly distiguasing C style
// string (null terminated).  [2024/05/26]
typedef const Char8  *ZStr8;
typedef const Char16 *ZStr16;

EXPECT_TYPE_SIZE(ZStr8,  POINTER_SIZE);
EXPECT_TYPE_SIZE(ZStr16, POINTER_SIZE);

#if !defined(__cplusplus)
    // TODO(gr3yknigh1): Typedef to _Bool if C11
    #if !defined(bool)
        typedef S8 bool;
    #endif

    #if !defined(true)
        #define true 1
    #endif // true

    #if !defined(false)
        #define false 0
    #endif // false

    #if !defined(NULL)
        #define NULL ((void *)0)
    #endif // NULL
#endif

EXPECT_TYPE_SIZE(bool, 1);


//
// Tilemaps:
//

struct Asset;


//! @note Two triangles.
constexpr U8 TILEMAP_VERTEX_COUNT_PER_TILE = 6;

struct Tilemap {
    //! @todo(gr3yknigh1): Implement uint parsing. [2025/02/23] #refactor #parsing

    int row_count;
    int col_count;

    int tile_x_pixel_count;
    int tile_y_pixel_count;

    int *indexes;
    size_t indexes_count;

    Asset *texture_asset;

    constexpr int tiles_count(void) noexcept { return this->row_count * this->col_count; }
};

//
// Debug (dbg):
//

//!
//! @brief Basicly replacement for `std::source_location`.
//!
//! @todo(gr3yknigh1) Maybe replace with `std::source_location`? [2025/04/25]
//!
struct Source_Location {
    //!
    //! @brief Name of the file (__FILE__).
    //!
    ZStr8 file_name;

    //!
    //! @brief Number of the line (__LINE__).
    //!
    U32 line;

    //!
    //! @brief Number of the column.
    //!
    U32 column;

    //!
    //! @brief Decorated function name (__FUNCDNAME__).
    //!
    ZStr8 function_name;


    constexpr Source_Location(std::source_location location) noexcept : file_name(location.file_name()), line(location.line()), column(location.column()), function_name(location.function_name()) {}
};


constexpr bool zstr8_is_equals(ZStr8 a, ZStr8 b) noexcept; // XXX

constexpr bool
source_location_is_equals(const Source_Location *a, const Source_Location *b) noexcept
{
    return (
        a->line == b->line
        && a->column && b->column
        && zstr8_is_equals(a->function_name, b->function_name)
        && zstr8_is_equals(a->file_name, b->file_name)
    );
}

//
// Graphics (gfx):
//

//! @todo(gr3yknigh1): Make it template? Or only `Rect` [2025/04/24] #refactor #renaming
struct Rect_F32 {
    F32 x;
    F32 y;

    //! @todo(gr3yknigh1): Rename this? [2025/04/24] #refactor #renaming
    F32 width;
    F32 height;
};

//!
//! @brief Struct which represents sprite atlas. Currently only holds information about count of pixels in Atlas.
//!
//! @todo(gr3yknigh1): Replace with integers? [2025/02/26] #refactor
//!
//! @todo(gr3yknigh1): Need to refactor this struct into something else [2025/04/24] #refactor
//!
struct Atlas {
    //!
    //! @brief Count of horizontal pixels (width).
    //!
    F32 x_pixel_count;

    //!
    //! @brief Count of vertical pixels (height).
    //!
    F32 y_pixel_count;
};

//! @todo(gr3yknigh1): Rename it. Maybe RGBA_PackU32 [2025/04/14] #refactor
typedef U32 packed_rgba_t;
EXPECT_TYPE_SIZE(packed_rgba_t, 4);

constexpr packed_rgba_t
pack_rgba_to_int(U8 r, U8 g, U8 b, U8 a)
{
    return ((r) << 24) | ((g) << 16) | ((b) << 8) | (a);
}

#pragma pack(push, 1)
struct Vertex {
    F32 x, y;
    F32 s, t;
    packed_rgba_t color;
};
#pragma pack(pop)
EXPECT_TYPE_SIZE(Vertex, sizeof(F32) * 4 + sizeof(packed_rgba_t));

#pragma pack(push, 1)
struct Color_RGBA_U8 {
    U8 r, g, b, a;
};
#pragma pack(pop)

typedef Color_RGBA_U8 Color4;

enum class Camera_ViewMode {
    Perspective,
    Orthogonal
};

struct Camera {
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;

    F32 yaw;
    F32 pitch;

    F32 speed;
    F32 sensitivity;
    F32 fov;

    F32 near;
    F32 far;

    Camera_ViewMode view_mode;
};

//!
//! @param[out] rect Output array of vertexes
//!
U32 generate_rect(Vertex *rect, F32 x, F32 y, F32 width, F32 height, Color4 color);

//!
//! @param[out] rect Output array of vertexes
//!
U32 generate_rect_with_atlas(
    Vertex *rect, F32 x, F32 y, F32 width, F32 height, Rect_F32 atlas_location, Atlas *altas, Color4 color);

//!
//! @param[out] vertexes Array of preallocated geometry-buffer to which this function will write.
//!
//! @param[in] tilemap Information about tilemap sizes, pixel-count and etc.
//!
//! @param[in] atlas Information about count of pixels on whole image (used for generating UVs properly).
//!
U32 generate_geometry_from_tilemap(Vertex *vertexes, U32 vertexes_capacity, Tilemap *tilemap, F32 origin_x, F32 origin_y, Color4 color, Atlas *atlas);

//
// MM (memory management):
//

namespace mm {

struct Buffer_View {
    Byte *data;
    USize size;

    constexpr Buffer_View() noexcept : data(nullptr), size(0) {}
};

USize get_page_size(void);

USize align(USize size, USize alignment);
USize page_align(USize size);

void zero_memory(void *p, USize size);
void copy_memory(void *dst, const void *src, USize size);

//!
//! @brief Make offset by number of bytes specified.
//!
template<typename Ty = void>
inline Ty *
get_offset(Ty *pointer, USize offset)
{
    return reinterpret_cast<Ty *>(reinterpret_cast<Byte *>(pointer) + offset);
}

template <typename Ty>
inline void
zero_struct(Ty *p)
{
    zero_memory(reinterpret_cast<void *>(p), sizeof(*p));
}

template <typename Ty>
inline void
zero_structs(Ty *p, U64 count)
{
    zero_memory(reinterpret_cast<void *>(p), sizeof(*p) * count);
}

//! @todo(gr3yknigh1): Replace with typed-enum class [2025/04/07] #refactor

#define ALLOCATE_NO_OPTS     MAKE_FLAG(0)
#define ALLOCATE_ZERO_MEMORY MAKE_FLAG(1)

typedef U32 Allocate_Options;

//!
//! @brief Base version of `allocate` function. Calls to platform specific allocation function.
//!
void *allocate(USize size, Allocate_Options options = ALLOCATE_NO_OPTS);

template <typename Ty>
inline Ty *
allocate_struct(Allocate_Options options = ALLOCATE_NO_OPTS)
{
    return static_cast<Ty *>(allocate(sizeof(Ty), options));
}

template <typename Ty>
inline Ty *
allocate_structs(U64 count, Allocate_Options options = ALLOCATE_NO_OPTS)
{
    return static_cast<Ty *>(allocate(sizeof(Ty) * count, options));
}


bool deallocate(void *p);

//!
//! @brief Memory block, which has static capacity, no ability to deallocate each individual allocations and can only be free whole block.
//!
struct Fixed_Arena {
    void *data;
    USize capacity;
    USize occupied;
};

Fixed_Arena  make_static_arena(USize capacity);
bool         destroy(Fixed_Arena *arena);
void *       allocate(Fixed_Arena *arena, USize size, Allocate_Options options = ALLOCATE_NO_OPTS);

template <typename Ty>
inline Ty *
allocate_structs(Fixed_Arena *arena, U64 count, Allocate_Options options = ALLOCATE_NO_OPTS)
{
    return static_cast<Ty *>(allocate(arena, sizeof(Ty) * count, options));
}

template <typename Ty>
inline Ty *
allocate_struct(Fixed_Arena *arena, Allocate_Options options = ALLOCATE_NO_OPTS)
{
    return static_cast<Ty *>(allocate(arena, sizeof(Ty), options));
}

//!
//! @brief Sets occupied field to zero. All allocated memory can be overwritten.
//!
//! @returns Number of bytes which was occupied.
//!
USize reset(Fixed_Arena *arena);

struct Stack_View {
    void *data;
    USize capacity;
    USize occupied;
};

bool can_hold(Stack_View *view, USize size);

bool reset(Stack_View *view);

void *allocate(Stack_View *view, USize size);

//!
//! @brief Initializes a view in stack-like data-block, and do not own it. Free it yourself!
//!
Stack_View make_stack_view(void *data, USize capacity);

//!
//! @brief Allocates new data-block and initializes a view in stack-like data-block, and do not own it. Free it yourself!
//!
Stack_View make_stack_view(USize capacity);

struct Block {
    Block *next;
    Block *previous;

    Stack_View stack;
};

struct Block_Allocator {
    //!
    //! @brief Basiclly is linked-list.
    //!
    struct {
        Block *head;
        Block *tail;
        U64 count;
    } blocks;

    //!
    //! @brief If greater than zero, tells the allocator to allocate blocks with fixed size, which makes it basiclly
    //! behave like pool allocator. If equals zero, blocks will be allocated with specified size aligned to page size.
    //!
    USize block_fixed_size;

    //!
    //! @brief If greater than zero, sets limit on count of block, which can be allocated. If zero, there will be no
    //! limit to block allocation.
    //!
    U64 block_count_limit;
};

Block_Allocator make_block_allocator();

//!
//! @brief Pre-allocates specified amount of blocks with specified size.
//!
Block_Allocator make_block_allocator(U64 blocks_count, USize block_size, USize block_fixed_size = 0, U64 block_count_limit = 0);

void *allocate(Block_Allocator *allocator, USize size, Allocate_Options options = ALLOCATE_NO_OPTS);

template <typename Ty>
inline Ty *
allocate_struct(Block_Allocator *allocator, Allocate_Options options = ALLOCATE_NO_OPTS)
{
    return static_cast<Ty *>(allocate(allocator, sizeof(Ty), options));
}

bool reset(Block_Allocator *allocator, void *data);

void *first(Block_Allocator *allocator);
void *next(Block_Allocator *allocator, void *data);

bool destroy_block_allocator(Block_Allocator *allocator);

struct Allocation_Record {
    Allocate_Options options;
    USize size;
    void *result;
    Source_Location location;
};

//!
//! @brief Returns pointer to allocation record list which was made with `allocate` function. [2025/04/25]
//!
//! @todo(gr3yknigh1): Make it per-thread. [2025/04/25]
//!
std::list<Allocation_Record> *get_allocation_records(void);

//!
//! @brief Dump all records into console.
//!
//! @return True if any allocations was dumped into console.
//!
//! @todo(gr3yknigh1): Add option to dump it all into disk. [2025/04/25]
//!
bool dump_allocation_records(bool do_hex_dump = false);

//!
//! @brief Dumps into console hex-view of the memory (formatted).
//!
//! @todo(gr3yknigh1): Add option to dump it all to other place (file for example) [2025/04/25]
//!
void hex_dump(void *buffer, U64 buffer_length);

struct Basic_Allocator {};

template<typename Allocator_Type>
constexpr bool
external_lifetime()
{
    return true;
}

template<>
constexpr bool
external_lifetime<Basic_Allocator>()
{
    return false;
}

inline void *
allocate([[maybe_unused]] Basic_Allocator *allocator, USize size, Allocate_Options options = ALLOCATE_NO_OPTS) noexcept
{
    return allocate(size, options);
}

inline bool
deallocate([[maybe_unused]] Basic_Allocator *allocator, void *data) noexcept
{
    return deallocate(data);
}

template <typename Ty>
inline Ty *
allocate_struct([[maybe_unused]] Basic_Allocator *allocator, Allocate_Options options = ALLOCATE_NO_OPTS)
{
    return static_cast<Ty *>(allocate(allocator, sizeof(Ty), options));
}

} // namespace mm

//! @todo(gr3yknigh1): Add namespace `sane`. [2025/04/24] #renaming

//
// Strings:
//

constexpr bool
zstr8_is_equals(ZStr8 a, ZStr8 b) noexcept
{
    while (a != nullptr && *a != 0 && b != nullptr && *b != 0) {
        if (*a != *b) {
            return false;
        }

        ++a;
        ++b;
    }

    if ((*a == 0 && *b != 0) || (*a != 0 && *b != 0)) {
        return false;
    }

    return true;
}

// TODO(gr3yknigh1): Rename str8_get_lentgh -> zstr8_get_length
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
        static_cast<Byte *>(destination)[index] = source[index];
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

bool
str8_is_equals(const Str8 *a, const Str8 *b) noexcept
{
    if (a->length != b->length) {
        return false;
    }

    // TODO(gr3yknigh1): Do vectorization [2025/01/03]
    for (U64 i = 0; i < a->length; ++i) {
        if (a->data[i] != b->data[i]) {
            return false;
        }
    }

    return true;
}

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

//
// Parsing:
//
struct Lexer {
    char *cursor;
    char lexeme;

    char *buffer;
    size_t buffer_size;
};

//!
//! @todo Make buffer be const!
//!
Lexer make_lexer(char *buffer, size_t buffer_size);

void lexer_advance(Lexer *lexer, int32_t count = 1);

char      lexer_peek(Lexer *lexer, int32_t offset);
Str8_View lexer_peek_view(Lexer *lexer, int32_t offset);

bool      lexer_check_peeked(Lexer *lexer, const char *s);
bool      lexer_check_peeked(Lexer *lexer, Str8_View sv);
bool      lexer_check_peeked_and_advance(Lexer *lexer, Str8_View sv);

bool      lexer_parse_int(Lexer *lexer, int *result);
bool      lexer_parse_str_to_view(Lexer *lexer, Str8_View *sv);

// TODO(gr3yknigh1): Rename lexer_skip_until* to lexer_advance_until* [2025/03/28]
Str8_View lexer_skip_until(Lexer *lexer, char c);
Str8_View lexer_skip_until(Lexer *lexer, Str8_View sv);
Str8_View lexer_skip_until_endline(Lexer *lexer);
void      lexer_skip_whitespace(Lexer *lexer);

bool      lexer_is_endline(Lexer *lexer, bool *is_crlf = nullptr);
bool      lexer_is_end(Lexer *lexer);

//
// Containers:
//

template <typename Value_Type, typename Allocator_Type = mm::Basic_Allocator>
struct Linked_List {

    struct Node {
        Node *next;
        Node *previous;

        Value_Type value;

        constexpr
        Node(const Value_Type &value_) noexcept
            : next(nullptr), previous(nullptr), value(value_)
        {
        }

        void
        chain(Node *next_node) noexcept
        {
            this->next = next_node;
            next_node->previous = this;
        }
    };

    struct Iterator {

        constexpr
        Iterator(Node *current_) noexcept
            : current(current_)
        {
        }

        Node *
        get_node(void) noexcept
        {
            return this->current;
        }

        Value_Type &
        operator*(void) const noexcept
        {
            return this->current->value;
        }

        Value_Type *
        operator->() noexcept
        {
            return &this->current->value;
        }

        virtual Iterator & operator++(void) noexcept = 0;

        Iterator
        operator++(int) noexcept
        {
            Iterator t = *this;
            ++(*this);
            return t;
        }

        friend bool
        operator== (const Iterator& a, const Iterator& b) noexcept
        {
            return a.current == b.current;
        };

        friend bool
        operator!= (const Iterator& a, const Iterator& b) noexcept
        {
            return a.current != b.current;
        };

        Node *current;
    };

    struct Forward_Iterator : public Iterator {

        constexpr
        Forward_Iterator(Node *current_) noexcept
            : Iterator(current_)
        {
        }

        Iterator &
        operator++(void) noexcept override
        {
            if (this->current != nullptr) {
                this->current = this->current->next;
            }
            return *this;
        }

    };

    struct Backward_Iterator : public Iterator {

        constexpr
        Backward_Iterator(Node *current_) noexcept
            : Iterator(current_)
        {
        }

        Iterator &
        operator++(void) noexcept override
        {
            if (this->current != nullptr) {
                this->current = this->current->previous;
            }
            return *this;
        }

    };

    Node *head;
    Node *tail;

    U64 count;

    Allocator_Type *allocator;

    constexpr
    Linked_List(void) noexcept
        : head(nullptr), tail(nullptr), count(0), allocator(nullptr)
    {
    }

    constexpr
    Linked_List(Allocator_Type *allocator) noexcept
        : head(nullptr), tail(nullptr), count(0), allocator(allocator)
    {
    }

    ~Linked_List(void) noexcept
    {
        if constexpr (!mm::external_lifetime<Allocator_Type>()) {
            this->clear();
        }
    }

    Linked_List(const Linked_List &) = delete;
    Linked_List(Linked_List &&) = delete;

    bool
    push_back(const Value_Type &new_element) noexcept
    {
        Node *new_node = mm::allocate_struct<Node>(this->allocator, ALLOCATE_ZERO_MEMORY);

        if (!new_node) {
            return false;
        }

        std::construct_at(new_node, new_element);

        if (this->head == nullptr) {
            assert(!this->count);
            this->head = new_node;
        } else {
            this->tail->chain(new_node);
        }

        this->tail = new_node;
        ++this->count;

        return true;
    }

    Value_Type &
    last(void) noexcept
    {
        return this->rbegin().current->value;
    }

    const Value_Type &
    last(void) const noexcept
    {
        return this->rbegin().current->value;
    }

    bool
    clear(void) noexcept
    {
        for (Forward_Iterator it = this->begin(); it != this->end(); ++it) {
            Node *current = it.get_node();
            if (current->previous) {
                mm::deallocate(this->allocator, current->previous);
            }
        }
        mm::deallocate(this->allocator, this->tail);
        mm::zero_memory(this, sizeof(*this));
        return true;
    }

    constexpr Forward_Iterator
    begin(void) noexcept
    {
        return Forward_Iterator(this->head);
    }

    constexpr Forward_Iterator
    end(void) noexcept
    {
        return Forward_Iterator(nullptr);
    }

    constexpr Backward_Iterator
    rbegin(void) noexcept
    {
        return Backward_Iterator(this->tail);
    }

    constexpr Backward_Iterator
    rend(void) noexcept
    {
        return Backward_Iterator(nullptr);
    }
};

//
// Error reporting:
//

enum class Severenity {
    None,
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
    _Count,
};

Char8 SEVERENITY_LETTERS[static_cast<USize>(Severenity::_Count)] {
    '?', 'T', 'D', 'I', 'W', 'E', 'F',
};

struct Report {
    Str8 message;
    Severenity severenity;
    Source_Location source_location;
    U64 count;
};

struct Reporter {
    Linked_List<Report> reports;

    void
    report(Severenity severenity, Str8_View message, Source_Location source_location = Source_Location(std::source_location::current())) noexcept
    {
        if (reports.count > 0) {
            Report &last_report = this->reports.last();

            if (source_location_is_equals(&source_location, &last_report.source_location)) {
                last_report.count++;
                return;
            }
        }

        reports.push_back(Report(Str8(message.data, message.length), severenity, source_location, 1));
    }
};

//
// Etc:
//

int get_offset_from_coords_of_2d_grid_array_rm(int width, int x, int y);

//
// OS:
//
USize get_file_size(FILE *file);

//
// Gameplay:
//

struct Input_State {
    F32 x_direction;
    F32 y_direction;
};


struct Platform_Context {
    Input_State input_state;
    Camera *camera;

    mm::Fixed_Arena persist_arena;

    // NOTE(gr3yknigh1): Platform runtime will call issue a draw call if vertexes_count > 0 [2025/03/03]
    mm::Fixed_Arena vertexes_arena;
    Vertex *vertexes;
    USize vertexes_count;
};
