//
// FILE          code\garden_platform.cpp
//
// AUTHORS
//               Ilya Akkuzin <gr3yknigh1@gmail.com>
//
// NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//

#include <assert.h>

#if defined(GARDEN_USE_CRT_ALLOCATIONS)

    #define _CRTDBG_MAP_ALLOC
    #include <stdlib.h>
    #include <crtdbg.h>

#endif

#include "garden_platform.h"

#include <Windows.h> // TODO(gr3yknigh1): Get rid of this include in gameplay code [2025/03/03]

// TODO(gr3yknigh1): Replace with more non-platform dependent code [2025/03/03]
#if !defined(ZERO_STRUCT)
    #define ZERO_STRUCT(STRUCT_PTR) ZeroMemory((STRUCT_PTR), sizeof(*(STRUCT_PTR)))
#endif

#if GARDEN_GAMEPLAY_CODE != 1
    #if defined(_WIN32)
        #include "garden_platform_win32.cpp"
    #else
        #error "Unhandled platform! No runtime was included"
    #endif
#endif

size_t
generate_rect(Vertex *vertexes, float x, float y, float width, float height, Color4 color)
{
    size_t c = 0;

    // bottom-left triangle
    vertexes[c++] = {x + 0, y + 0, 0, 0, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a)};          // bottom-left
    vertexes[c++] = {x + width, y + 0, 1, 0, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a)};      // bottom-right
    vertexes[c++] = {x + width, y + height, 1, 1, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a)}; // top-right

    // top-right triangle
    vertexes[c++] = {x + 0, y + 0, 0, 0, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a)};          // bottom-left
    vertexes[c++] = {x + width, y + height, 1, 1, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a)}; // top-right
    vertexes[c++] = {x + 0, y + height, 0, 1, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a)};

    return c;
}

// TODO(gr3yknigh1): Separate configuration at atlas and mesh size [2025/02/26]
size_t
generate_rect_with_atlas(
    Vertex *vertexes, float x, float y, float width, float height, Rect_F32 loc, Atlas *atlas, Color4 color)
{
    size_t c = 0;

    // bottom-left triangle
    vertexes[c++] = {
        x + 0, y + 0, (loc.x + 0) / atlas->x_pixel_count, (loc.y + 0) / atlas->y_pixel_count,
        MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a)}; // bottom-left
    vertexes[c++] = {
        x + width, y + 0, (loc.x + loc.width) / atlas->x_pixel_count, (loc.y + 0) / atlas->y_pixel_count,
        MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a)}; // bottom-right
    vertexes[c++] = {
        x + width, y + height, (loc.x + loc.width) / atlas->x_pixel_count, (loc.y + loc.height) / atlas->y_pixel_count,
        MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a)}; // top-right

    // top-right triangle
    vertexes[c++] = {
        x + 0, y + 0, (loc.x + 0) / atlas->x_pixel_count, (loc.y + 0) / atlas->y_pixel_count,
        MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a)}; // bottom-left
    vertexes[c++] = {
        x + width, y + height, (loc.x + loc.width) / atlas->x_pixel_count, (loc.y + loc.height) / atlas->y_pixel_count,
        MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a)}; // top-right
    vertexes[c++] = {
        x + 0, y + height, (loc.x + 0) / atlas->x_pixel_count, (loc.y + loc.height) / atlas->y_pixel_count,
        MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a)};

    return c;
}

mm::Arena
mm::make_arena(size_t capacity)
{
    mm::Arena arena;

    arena.data = mm::allocate(capacity);
    arena.capacity = capacity;
    arena.occupied = 0;

    return arena;
}

bool
mm::arena_pop(mm::Arena *arena, void *data)
{
    if (arena == nullptr || data == nullptr) {
        return false;
    }

    if (arena->data == nullptr || arena->occupied < sizeof(arena->occupied) || arena->capacity == 0) {
        return false;
    }

    size_t *data_size = static_cast<size_t *>(data) - 1;

    assert(static_cast<mm::byte *>(data) + *data_size == static_cast<mm::byte *>(arena->data) + arena->occupied);
    arena->occupied -= *data_size;
    arena->occupied -= sizeof(*data_size);

    return true;
}

size_t
mm::arena_reset(mm::Arena *arena)
{
    size_t was_occupied = arena->occupied;
    arena->occupied = 0;
    return was_occupied;
}

void *
mm::allocate(Arena *arena, size_t size)
{
    return mm::arena_alloc(arena, size, ARENA_ALLOC_BASIC);
}

// TODO(gr3yknigh1): Replace with mm::allocate [2025/03/09]
void *
mm::arena_alloc(mm::Arena *arena, size_t size, int options)
{
    if (arena == nullptr) {
        return nullptr;
    }

    size_t additionals_size = 0;

    if (HAS_FLAG(options, ARENA_ALLOC_POPABLE)) {
        additionals_size = sizeof(size);
    }

    if (arena->occupied + size + additionals_size > arena->capacity) {
        return nullptr;
    }

    void *allocated = static_cast<char *>(arena->data) + arena->occupied;

    if (HAS_FLAG(options, ARENA_ALLOC_POPABLE)) {
        *(static_cast<size_t *>(allocated)) = size;

        allocated = static_cast<mm::byte *>(allocated) + additionals_size;
    }

    arena->occupied += size + additionals_size;
    return allocated;
}

void *
mm::arena_alloc_set(mm::Arena *arena, size_t size, char c, int options)
{
    void *allocated = mm::arena_alloc(arena, size, options);

    if (allocated == nullptr) {
        return allocated;
    }

    memset(allocated, c, size);
    return allocated;
}

void *
mm::arena_alloc_zero(mm::Arena *arena, size_t size, int options)
{
    return mm::arena_alloc_set(arena, size, 0, options);
}

bool
mm::free_arena(mm::Arena *arena)
{
    bool result = mm::deallocate(arena->data);
    ZERO_STRUCT(arena);
    return result;
}

void
mm::zero_memory(void *p, size_t size)
{
    for (size_t byte_index = 0; byte_index < size; ++byte_index) {
        static_cast<mm::byte *>(p)[byte_index] = 0;
    }
}

void *
mm::allocate(size_t size)
{
    void *result = nullptr;

#if defined(GARDEN_USE_CRT_ALLOCATIONS)
    result = malloc(size);
#else
    result = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#endif

    return result;
}

bool
mm::deallocate(void *p)
{
    bool result = true;

#if defined(GARDEN_USE_CRT_ALLOCATIONS)
    free(p);
#else
    result = VirtualFree(p, 0, MEM_RELEASE);
#endif
    return result;
}

mm::Block_Allocator
mm::make_block_allocator()
{
    mm::Block_Allocator result;
    mm::zero_struct(&result);
    return result;
}

mm::Block_Allocator
mm::make_block_allocator(uint64_t blocks_count, size_t block_size, size_t block_fixed_size, uint64_t block_count_limit)
{
    assert(blocks_count && block_size);

    if (block_count_limit > 0) {
        assert(blocks_count <= block_count_limit);
    }

    if (block_fixed_size > 0) {
        assert(block_size == block_fixed_size);
    }

    mm::Block_Allocator result;
    mm::zero_struct(&result);

    result.block_fixed_size = block_fixed_size;
    result.block_count_limit = block_count_limit;
    result.blocks.count = blocks_count;

    // TODO(gr3yknigh1): Allocate each block near the actuall data? [2025/03/09]

    if (block_count_limit) {
        result.blocks.head = mm::allocate_structs<mm::Block>(blocks_count);
        mm::zero_structs(result.blocks.head, blocks_count);
    } else {
        result.blocks.head = mm::allocate_struct<mm::Block>();
        mm::zero_struct(result.blocks.head);
    }
    assert(result.blocks.head);

    mm::byte *data_cursor = nullptr;
    if (block_fixed_size) {
        data_cursor = static_cast<mm::byte *>(mm::allocate(blocks_count * block_size));
    }

    mm::Block *current_block = result.blocks.head;

    for (uint64_t block_index = 0; block_index < blocks_count; ++block_index) {

        if (block_fixed_size) {
            current_block->stack = mm::make_stack_view(data_cursor, block_size);
        } else {
            current_block->stack = mm::make_stack_view(block_size);
        }

        if (block_index == blocks_count - 1) {
            result.blocks.tail = current_block;
            continue;
        }

        mm::Block *next_block = nullptr;

        if (block_count_limit) {
            next_block = result.blocks.head + block_index + 1;
        } else {
            next_block = mm::allocate_struct<mm::Block>();
        }

        current_block->next = next_block;
        next_block->previous = current_block;


        if (block_fixed_size) {
            data_cursor += block_size;
        }

        current_block = current_block->next;
    }

    return result;
}



void *
mm::allocate(mm::Block_Allocator *allocator, size_t size)
{
    assert(allocator && size);

    if (allocator->block_fixed_size != 0) {
        assert(size == allocator->block_fixed_size);
    }

    FOR_LINKED_LIST(it, &allocator->blocks) {
        void *result = mm::allocate(&it->stack, size);
        if (result != nullptr) {
            return result;
        }
    }

    //
    // Block count limit is exceeded
    //
    if (allocator->block_count_limit && allocator->block_fixed_size) {
        //                              ^^^^^^^^^^^^^^^^^^^^^^^^^^^
        // TODO(gr3yknigh1): Wrap `mm::Block` in allocation-chunks (likewise data blobs). Because we can't free
        // such blocks and semantics can allow only make `Pool`-like allocations or variable-size blocks, where each
        // block and data is a separate allocations. [2025/03/10]
        //
        // TODO(gr3yknigh1): Try make block and data-blob allocation joined, when only `block_fixed_size` parameter is
        // specified [2025/03/10]
        //

        // All blocks should be preallocated
        assert(allocator->blocks.count == allocator->block_count_limit);
        return nullptr;
    }

    //
    // NOTE(gr3yknigh1): Can't pre-allocate more blocks, because semantics will not allow specify block_size
    // for preallocated blocks for grow. We need add function like `mm::block_allocator_pre_allocate(block_size, block_count);` in order to make it more compact.
    // [2025/03/10]
    //
    mm::Block *new_block = mm::allocate_struct<mm::Block>();
    mm::zero_struct(new_block);

    allocator->blocks.tail->next = new_block;
    allocator->blocks.tail = new_block;
    allocator->blocks.count++;

    if (allocator->block_fixed_size) {
        new_block->stack = mm::make_stack_view(allocator->block_fixed_size);
    } else {
        new_block->stack = mm::make_stack_view(mm::page_align(size));
    }

    void *result = mm::allocate(&new_block->stack, size);
    return result;
}


bool
mm::deallocate(mm::Block_Allocator *allocator)
{
    assert(allocator);

    if (!allocator->blocks.count) {
        return true;
    }

    assert((allocator->block_fixed_size && allocator->block_count_limit)
        || (!allocator->block_fixed_size && !allocator->block_count_limit));

    bool result = true;

    if (allocator->block_fixed_size) {
        result = mm::deallocate( static_cast<void *>( allocator->blocks.head->stack.data ) );
        if (result) {
            result = mm::deallocate( static_cast<void *>( allocator->blocks.head ) );
        }
    } else {

        FOR_LINKED_LIST(it, &allocator->blocks) {
            result = mm::deallocate(it->stack.data);

            if (result && it->previous != nullptr) {
                result = mm::deallocate( static_cast<void *>( it->previous ) );
            }

            if (result && it->next == nullptr) {
                result = mm::deallocate( static_cast<void *>( it ) );
                break;
            }

            if (!result) {
                break;
            }
        }
    }


    return result;
}

bool
mm::can_hold(mm::Stack_View *view, size_t size)
{
    assert(view && size);

    bool result = (size + view->occupied) <= view->capacity;
    return result;
}

void *
mm::allocate(mm::Stack_View *view, size_t size)
{
    if (!mm::can_hold(view, size)) {
        return nullptr;
    }

    void *result = static_cast<mm::byte *>(view->data) + view->occupied;
    view->occupied += size;

    return result;
}

mm::Stack_View
mm::make_stack_view(void *data, size_t capacity)
{
    mm::Stack_View result;
    result.data = data;
    result.capacity = capacity;
    result.occupied = 0;

    return result;
}

mm::Stack_View
mm::make_stack_view(size_t capacity)
{
    mm::Stack_View result;
    result.data = mm::allocate(capacity);
    result.capacity = capacity;
    result.occupied = 0;
    return result;
}

size_t
mm::align(size_t size, size_t alignment)
{
    size_t result =  size + (alignment - size % alignment);
    return result;
}

size_t
mm::page_align(size_t size)
{
    size_t page_size = mm::get_page_size();
    size_t result = mm::align(size, page_size);
    return result;
}

size_t
mm::get_page_size()
{
    size_t result;
    SYSTEM_INFO systemInfo = {0};
    GetSystemInfo(&systemInfo);
    result = systemInfo.dwPageSize;
    return result;
}
