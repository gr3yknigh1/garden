//!
//! FILE          code\base\memory.h
//!
//! AUTHORS
//!               Ilya Akkuzin <gr3yknigh1@gmail.com>
//!
//! NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//!
#pragma once

#include "base/types.h"
#include "base/macros.h"

struct Buffer_View {
    Byte *data;
    Size  size;

    constexpr Buffer_View() noexcept : data(nullptr), size(0) {}
};

Size get_page_size(void);

Size align(Size size, Size alignment);
Size page_align(Size size);

void zero_memory(void *p, Size size);
void copy_memory(void *dst, const void *src, Size size);

//!
//! @brief Make offset by number of bytes specified.
//!
inline void *
get_offset(void *pointer, Size offset)
{
    return static_cast<Byte *>(pointer) + offset;
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

// TODO(gr3yknigh1): Replace with typed-enum class [2025/04/07]
typedef I32 Allocate_Options;

#define ALLOCATE_NO_OPTS        MAKE_FLAG(0)
#define ALLOCATE_ZERO_MEMORY    MAKE_FLAG(1)

//
// @brief Base version of `allocate` function. Calls to platform specific allocation function.
//
void *allocate(Size size, Allocate_Options options = ALLOCATE_NO_OPTS);

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
struct Static_Arena {
    void *data;
    Size capacity;
    Size occupied;
};

Static_Arena make_static_arena(Size capacity);
bool         destroy(Static_Arena *arena);

void *       allocate(Static_Arena *arena, Size size, Allocate_Options options = ALLOCATE_NO_OPTS);

template <typename Ty>
inline Ty *
allocate_structs(Static_Arena *arena, U64 count, Allocate_Options options = ALLOCATE_NO_OPTS)
{
    return static_cast<Ty *>(allocate(arena, sizeof(Ty) * count, options));
}

template <typename Ty>
inline Ty *
allocate_struct(Static_Arena *arena, Allocate_Options options = ALLOCATE_NO_OPTS)
{
    return allocate_structs<Ty>(arena, 1, options);
}

//!
//! @brief Sets occupied field to zero. All allocated memory can be overwritten.
//!
//! @returns Number of bytes which was occupied.
//!
Size reset(Static_Arena *arena);

struct Stack_View {
    void *data;
    Size capacity;
    Size occupied;
};

bool can_hold(Stack_View *view, Size size);

bool reset(Stack_View *view);

void *allocate(Stack_View *view, Size size);

//
// @brief Initializes a view in stack-like data-block, and do not own it. Free it yourself!
//
Stack_View make_stack_view(void *data, Size capacity);

//
// @brief Allocates new data-block and initializes a view in stack-like data-block, and do not own it. Free it yourself!
//
Stack_View make_stack_view(Size capacity);

struct Block {
    Block *next;
    Block *previous;

    Stack_View stack;
};

struct Block_Allocator {

    //
    // @brief Basiclly is linked-list.
    //
    struct {
        Block *head;
        Block *tail;
        U64 count;
    } blocks;

    //
    // @brief If greater than zero, tells the allocator to allocate blocks with fixed size, which makes it basiclly
    // behave like pool allocator. If equals zero, blocks will be allocated with specified size aligned to page size.
    //
    Size block_fixed_size;

    //
    // @brief If greater than zero, sets limit on count of block, which can be allocated. If zero, there will be no
    // limit to block allocation.
    //
    U64 block_count_limit;
};

Block_Allocator make_block_allocator();

//
// @brief Pre-allocates specified amount of blocks with specified size.
//
Block_Allocator make_block_allocator(U64 blocks_count, Size block_size, Size block_fixed_size = 0, U64 block_count_limit = 0);

void *allocate(Block_Allocator *allocator, Size size, Allocate_Options options = ALLOCATE_NO_OPTS);

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
