//
// FILE          code\garden_platform.cpp
//
// AUTHORS
//               Ilya Akkuzin <gr3yknigh1@gmail.com>
//
// NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//

#include <assert.h>

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
    vertexes[c++] = { x + 0    , y + 0,      0, 0, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // bottom-left
    vertexes[c++] = { x + width, y + 0,      1, 0, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // bottom-right
    vertexes[c++] = { x + width, y + height, 1, 1, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // top-right

    // top-right triangle
    vertexes[c++] = { x + 0    , y + 0,      0, 0, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // bottom-left
    vertexes[c++] = { x + width, y + height, 1, 1, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // top-right
    vertexes[c++] = { x + 0,     y + height, 0, 1, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };

    return c;
}

// TODO(gr3yknigh1): Separate configuration at atlas and mesh size [2025/02/26]
size_t
generate_rect_with_atlas(Vertex *vertexes, float x, float y, float width, float height, Rect_F32 loc, Atlas *atlas, Color4 color)
{
    size_t c = 0;


    // bottom-left triangle
    vertexes[c++] = { x + 0    , y + 0,      (loc.x + 0)         / atlas->x_pixel_count, (loc.y + 0)          / atlas->y_pixel_count, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // bottom-left
    vertexes[c++] = { x + width, y + 0,      (loc.x + loc.width) / atlas->x_pixel_count, (loc.y + 0)          / atlas->y_pixel_count, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // bottom-right
    vertexes[c++] = { x + width, y + height, (loc.x + loc.width) / atlas->x_pixel_count, (loc.y + loc.height) / atlas->y_pixel_count, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // top-right

    // top-right triangle
    vertexes[c++] = { x + 0    , y + 0,      (loc.x + 0)         / atlas->x_pixel_count, (loc.y + 0)          / atlas->y_pixel_count, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // bottom-left
    vertexes[c++] = { x + width, y + height, (loc.x + loc.width) / atlas->x_pixel_count, (loc.y + loc.height) / atlas->y_pixel_count, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // top-right
    vertexes[c++] = { x + 0,     y + height, (loc.x + 0)         / atlas->x_pixel_count, (loc.y + loc.height) / atlas->y_pixel_count, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };

    return c;
}


Arena
make_arena(size_t capacity)
{
    Arena arena;

    arena.data = VirtualAlloc(
        0, capacity,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE);

    arena.capacity = capacity;
    arena.occupied = 0;

    return arena;
}

bool
arena_pop(Arena *arena, void *data)
{
    if (arena == nullptr || data == nullptr) {
        return false;
    }

    if (arena->data == nullptr || arena->occupied < sizeof(arena->occupied) || arena->capacity == 0) {
        return false;
    }

    size_t *data_size = ((size_t *)data) - 1;

    assert((char *)data + *data_size == (char *)arena->data + arena->occupied);
    arena->occupied -= *data_size;
    arena->occupied -= sizeof(*data_size);

    return true;
}

size_t
arena_reset(Arena *arena)
{
    size_t was_occupied = arena->occupied;
    arena->occupied = 0;
    return was_occupied;
}

void *
arena_alloc(Arena *arena, size_t size, int options)
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

    void *allocated = ((char *)arena->data) + arena->occupied;

    if (HAS_FLAG(options, ARENA_ALLOC_POPABLE)) {
        *((size_t *)allocated) = size;

        allocated = (char *)allocated + additionals_size;
    }

    arena->occupied += size + additionals_size;
    return allocated;
}

void *
arena_alloc_set(Arena *arena, size_t size, char c, int options)
{
    void *allocated = arena_alloc(arena, size, options);

    if (allocated == nullptr) {
        return allocated;
    }

    memset(allocated, c, size);
    return allocated;
}

void *
arena_alloc_zero(Arena *arena, size_t size, int options)
{
    return arena_alloc_set(arena, size, 0, options);
}

bool
free_arena(Arena *arena)
{
    bool result = VirtualFree(arena->data, 0, MEM_RELEASE);
    ZERO_STRUCT(arena);
    return result;
}
