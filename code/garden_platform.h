#if !defined(GARDEN_PLATFORM_H)
//
// FILE          code\platform.h
//
// AUTHORS
//               Ilya Akkuzin <gr3yknigh1@gmail.com>
//
// NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//
#define GARDEN_PLATFORM_H

#include <stdint.h>


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

//
// Allocators:
//

struct Arena {
    void *data;
    size_t capacity;
    size_t occupied;
};

Arena make_arena(size_t capacity);
bool  free_arena(Arena *arena);

#define ARENA_ALLOC_BASIC   MAKE_FLAG(0)
#define ARENA_ALLOC_POPABLE MAKE_FLAG(1)

void * arena_alloc(Arena *arena, size_t size, int options);
void * arena_alloc_zero(Arena *arena, size_t size, int options);
bool   arena_pop(Arena *arena, void *data);
size_t arena_reset(Arena *arena);

//
// Graphics:
//

typedef int packed_rgba_t;

static_assert(sizeof(packed_rgba_t) == 4);

#if !defined(MAKE_PACKED_RGBA)
    #define MAKE_PACKED_RGBA(R, G, B, A) (((R) << 24) + ((G) << 16) + ((B) << 8) + (A))
#endif

#pragma pack(push, 1)
struct Vertex {
    float x, y;
    float s, t;
    packed_rgba_t color;
};
#pragma pack(pop)

static_assert(sizeof(Vertex) == (sizeof(float) * 4 + sizeof(packed_rgba_t)));

#pragma pack(push, 1)
struct Color_RGBA_U8 {
    uint8_t r, g, b, a;
};
#pragma pack(pop)

typedef Color_RGBA_U8 Color4;

struct Rect_F32 {
    float x;
    float y;
    float width;
    float height;
};

// TODO(gr3yknigh1): Replace with integers? [2025/02/26]
struct Atlas {
    float x_pixel_count;
    float y_pixel_count;
};

struct Input_State {
    float x_direction;
    float y_direction;
};

struct Platform_Context {
    Input_State input_state;

    Arena persist_arena;

    // NOTE(gr3yknigh1): Platform runtime will call issue a draw call if vertexes_count > 0 [2025/03/03]
    Arena   vertexes_arena;
    Vertex *vertexes;
    size_t  vertexes_count;
};

//
// @param[out] rect Output array of vertexes
//
size_t generate_rect(Vertex *rect, float x, float y, float width, float height, Color4 color);

//
// @param[out] rect Output array of vertexes
//
size_t generate_rect_with_atlas(Vertex *rect, float x, float y, float width, float height, Rect_F32 atlas_location, Atlas *altas, Color4 color);

#endif // GARDEN_PLATFORM_H