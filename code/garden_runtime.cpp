//
// FILE          code\garden_platform.cpp
//
// AUTHORS
//               Ilya Akkuzin <gr3yknigh1@gmail.com>
//
// NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "garden_runtime.h"

#if GARDEN_GAMEPLAY_CODE != 1
    #if defined(_WIN32)
        #include "garden_runtime_win32.cpp"
    #else
        #error "Unhandled platform! No runtime was included"
    #endif
#endif

Size
generate_rect(Vertex *vertexes, F32 x, F32 y, F32 width, F32 height, Color4 color)
{
    Size c = 0;

    // bottom-left triangle
    vertexes[c++] = {x + 0, y + 0, 0, 0, pack_rgba_to_int(color.r, color.g, color.b, color.a)};          // bottom-left
    vertexes[c++] = {x + width, y + 0, 1, 0, pack_rgba_to_int(color.r, color.g, color.b, color.a)};      // bottom-right
    vertexes[c++] = {x + width, y + height, 1, 1, pack_rgba_to_int(color.r, color.g, color.b, color.a)}; // top-right

    // top-right triangle
    vertexes[c++] = {x + 0, y + 0, 0, 0, pack_rgba_to_int(color.r, color.g, color.b, color.a)};          // bottom-left
    vertexes[c++] = {x + width, y + height, 1, 1, pack_rgba_to_int(color.r, color.g, color.b, color.a)}; // top-right
    vertexes[c++] = {x + 0, y + height, 0, 1, pack_rgba_to_int(color.r, color.g, color.b, color.a)};

    return c;
}

// TODO(gr3yknigh1): Separate configuration at atlas and mesh size [2025/02/26]
Size
generate_rect_with_atlas(
    Vertex *vertexes, F32 x, F32 y, F32 width, F32 height, Rect_F32 loc, Atlas *atlas, Color4 color)
{
    Size c = 0;

    // bottom-left triangle
    vertexes[c++] = {
        x + 0, y + 0, (loc.x + 0) / atlas->x_pixel_count, (loc.y + 0) / atlas->y_pixel_count,
        pack_rgba_to_int(color.r, color.g, color.b, color.a)}; // bottom-left
    vertexes[c++] = {
        x + width, y + 0, (loc.x + loc.width) / atlas->x_pixel_count, (loc.y + 0) / atlas->y_pixel_count,
        pack_rgba_to_int(color.r, color.g, color.b, color.a)}; // bottom-right
    vertexes[c++] = {
        x + width, y + height, (loc.x + loc.width) / atlas->x_pixel_count, (loc.y + loc.height) / atlas->y_pixel_count,
        pack_rgba_to_int(color.r, color.g, color.b, color.a)}; // top-right

    // top-right triangle
    vertexes[c++] = {
        x + 0, y + 0, (loc.x + 0) / atlas->x_pixel_count, (loc.y + 0) / atlas->y_pixel_count,
        pack_rgba_to_int(color.r, color.g, color.b, color.a)}; // bottom-left
    vertexes[c++] = {
        x + width, y + height, (loc.x + loc.width) / atlas->x_pixel_count, (loc.y + loc.height) / atlas->y_pixel_count,
        pack_rgba_to_int(color.r, color.g, color.b, color.a)}; // top-right
    vertexes[c++] = {
        x + 0, y + height, (loc.x + 0) / atlas->x_pixel_count, (loc.y + loc.height) / atlas->y_pixel_count,
        pack_rgba_to_int(color.r, color.g, color.b, color.a)};

    return c;
}

Static_Arena
make_static_arena(Size capacity)
{
    Static_Arena arena;

    arena.data     = allocate(capacity);
    arena.capacity = capacity;
    arena.occupied = 0;

    return arena;
}

#if 0
// TODO(gr3yknigh1): Reuse for stack [2025/04/07]
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
#endif

Size
reset(Static_Arena *arena)
{
    Size was_occupied = arena->occupied;
    arena->occupied = 0;
    return was_occupied;
}

void *
allocate(Static_Arena *arena, Size size, Allocate_Options options)
{
    if (arena == nullptr) {
        return nullptr;
    }

    if (arena->occupied + size > arena->capacity) {
        return nullptr;
    }

    void *allocated = get_offset(arena->data, arena->occupied);

    if (HAS_FLAG(options, ALLOCATE_ZERO_MEMORY)) {
        zero_memory(allocated, size);
    }

    arena->occupied += size;
    return allocated;
}

bool
destroy(Static_Arena *arena)
{
    bool result = deallocate(arena->data);
    zero_struct<Static_Arena>(arena);
    return result;
}

void
zero_memory(void *p, Size size)
{
    std::memset(p, 0, size);
}

void
copy_memory(void *dst, const void *src, Size size)
{
    std::memcpy(dst, src, size);
}

void *
allocate(size_t size, Allocate_Options options)
{
    void *result = std::malloc(size);
    if (result && HAS_FLAG(options, ALLOCATE_ZERO_MEMORY)) {
        zero_memory(result, size);
    }
    return result;
}

bool
deallocate(void *p)
{
    // TODO(gr3yknigh1): Use platform functions for allocations [2025/04/07]
    std::free(p);
    return true;
}

Block_Allocator
make_block_allocator(void)
{
    Block_Allocator result;
    zero_struct(&result);
    return result;
}

Block_Allocator
make_block_allocator(U64 blocks_count, Size block_size, Size block_fixed_size, U64 block_count_limit)
{
    assert(blocks_count && block_size);

    if (block_count_limit > 0) {
        assert(blocks_count <= block_count_limit);
    }

    if (block_fixed_size > 0) {
        assert(block_size == block_fixed_size);
    }

    Block_Allocator result;
    zero_struct(&result);

    result.block_fixed_size = block_fixed_size;
    result.block_count_limit = block_count_limit;
    result.blocks.count = blocks_count;

    // TODO(gr3yknigh1): Allocate each block near the actuall data? [2025/03/09]

    if (block_count_limit) {
        result.blocks.head = allocate_structs<Block>(blocks_count);
        zero_structs(result.blocks.head, blocks_count);
    } else {
        result.blocks.head = allocate_struct<Block>();
        zero_struct(result.blocks.head);
    }
    assert(result.blocks.head);

    Byte *data_cursor = nullptr;
    if (block_fixed_size) {
        data_cursor = static_cast<Byte *>(allocate(blocks_count * block_size));
    }

    Block *current_block = result.blocks.head;

    for (U64 block_index = 0; block_index < blocks_count; ++block_index) {

        if (block_fixed_size) {
            current_block->stack = make_stack_view(data_cursor, block_size);
        } else {
            current_block->stack = make_stack_view(block_size);
        }

        if (block_index == blocks_count - 1) {
            result.blocks.tail = current_block;
            continue;
        }

        Block *next_block = nullptr;

        if (block_count_limit) {
            next_block = result.blocks.head + block_index + 1;
        } else {
            next_block = allocate_struct<Block>();
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
allocate(Block_Allocator *allocator, Size size, Allocate_Options options)
{
    assert(allocator && size);

    if (allocator->block_fixed_size != 0) {
        assert(size == allocator->block_fixed_size);
    }

    FOR_LINKED_LIST(it, &allocator->blocks) {
        void *result = allocate(&it->stack, size);
        if (result != nullptr) {

            if (HAS_FLAG(options, ALLOCATE_ZERO_MEMORY)) {
                zero_memory(result, size);
            }

            return result;
        }
    }

    //
    // Block count limit is exceeded
    //
    if (allocator->block_count_limit && allocator->block_fixed_size) {
        //                              ^^^^^^^^^^^^^^^^^^^^^^^^^^^
        // TODO(gr3yknigh1): Wrap `Block` in allocation-chunks (likewise data blobs). Because we can't free
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
    // for preallocated blocks for grow. We need add function like `block_allocator_pre_allocate(block_size, block_count);` in order to make it more compact.
    // [2025/03/10]
    //
    Block *new_block = allocate_struct<Block>();
    zero_struct(new_block);

    if (allocator->blocks.count > 0) {
        allocator->blocks.tail->next = new_block;
    } else {
        allocator->blocks.head = new_block;
    }

    allocator->blocks.tail = new_block;
    allocator->blocks.count++;

    if (allocator->block_fixed_size) {
        new_block->stack = make_stack_view(allocator->block_fixed_size);
    } else {
        new_block->stack = make_stack_view(page_align(size));
    }

    void *result = allocate(&new_block->stack, size);

    if (result && HAS_FLAG(options, ALLOCATE_ZERO_MEMORY)) {
        zero_memory(result, size);
    }

    return result;
}


bool
destroy_block_allocator(Block_Allocator *allocator)
{
    assert(allocator);

    if (!allocator->blocks.count) {
        return true;
    }

    assert((allocator->block_fixed_size && allocator->block_count_limit)
        || (!allocator->block_fixed_size && !allocator->block_count_limit));

    bool result = true;

    if (allocator->block_fixed_size) {
        result = deallocate( static_cast<void *>( allocator->blocks.head->stack.data ) );
        if (result) {
            result = deallocate( static_cast<void *>( allocator->blocks.head ) );
        }
    } else {

        FOR_LINKED_LIST(it, &allocator->blocks) {
            result = deallocate(it->stack.data);

            if (result && it->previous != nullptr) {
                result = deallocate( static_cast<void *>( it->previous ) );
            }

            if (result && it->next == nullptr) {
                result = deallocate( static_cast<void *>( it ) );
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
can_hold(Stack_View *view, Size size)
{
    assert(view && size);
    bool result = (size + view->occupied) <= view->capacity;
    return result;
}

void *
allocate(Stack_View *view, Size size)
{
    if (!can_hold(view, size)) {
        return nullptr;
    }

    void *result = get_offset(view->data, view->occupied);
    view->occupied += size;
    return result;
}

Stack_View
make_stack_view(void *data, Size capacity)
{
    Stack_View result;
    result.data = data;
    result.capacity = capacity;
    result.occupied = 0;

    return result;
}

Stack_View
make_stack_view(Size capacity)
{
    Stack_View result;
    result.data = allocate(capacity);
    result.capacity = capacity;
    result.occupied = 0;
    return result;
}

Size
align(Size size, Size alignment)
{
    Size result =  size + (alignment - size % alignment);
    return result;
}

Size
page_align(Size size)
{
    Size page_size = get_page_size();

    Size result = align(size, page_size);
    return result;
}

bool
reset(Stack_View *view)
{
    view->occupied = 0;
    return true;
}

bool
reset(Block_Allocator *allocator, void *data)
{
    bool result = false;

    FOR_LINKED_LIST(it, &allocator->blocks) {
        if (it->stack.data == data) {
            result = reset(&it->stack);
            break;
        }
    }

    return result;
}

void *
first(Block_Allocator *allocator)
{
    assert(allocator);

    if (!allocator->blocks.count) {
        return nullptr;
    }

    void *result = allocator->blocks.head->stack.data;
    return result;
}

void *
next(Block_Allocator *allocator, void *data)
{
    assert(allocator && data);

    // NOTE(gr3yknigh1): Very inefficient [2025/03/10]
    // @perf

    FOR_LINKED_LIST(it, &allocator->blocks) {
        if (it->stack.data == data && it->next != nullptr) {
            return it->next->stack.data;
        }
    }
    return nullptr;
}

Size
get_file_size(FILE *file)
{
    assert(file);

    fseek(file, 0, SEEK_END);
    Size file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    assert(file_size);

    return file_size;
}


#include <Windows.h>

Size
get_page_size(void)
{
    Size result;
    SYSTEM_INFO system_info = {0};
    GetSystemInfo(&system_info);
    result = system_info.dwPageSize;
    return result;
}
