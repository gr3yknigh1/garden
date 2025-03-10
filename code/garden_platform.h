#if !defined(GARDEN_PLATFORM_H)
//
// FILE          code\garden_platform.h
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

#if !defined(FOR_LINKED_LIST)
    #define FOR_LINKED_LIST(ITER, LINKED_LIST)                                         \
        for (auto ITER = (LINKED_LIST)->head; ITER != nullptr; ITER = ITER->next)
#endif

namespace mm
{

    using byte = char;

    size_t get_page_size();

    size_t align(size_t size, size_t alignment);
    size_t page_align(size_t size);

    void zero_memory(void *p, size_t size);

    template <typename Ty>
    inline void
    zero_struct(Ty *p)
    {
        mm::zero_memory(reinterpret_cast<void *>(p), sizeof(*p));
    }

    template <typename Ty>
    inline void
    zero_structs(Ty *p, uint64_t count)
    {
        mm::zero_memory(reinterpret_cast<void *>(p), sizeof(*p) * count);
    }

    //
    // @brief Base version of `mm::allocate` function. Calls to platform specific allocation function.
    //
    void *allocate(size_t size);

    template <typename Ty>
    inline Ty *
    allocate_struct()
    {
        return static_cast<Ty *>(mm::allocate(sizeof(Ty)));
    }

    template <typename Ty>
    inline Ty *
    allocate_structs(uint64_t count)
    {
        return static_cast<Ty *>(mm::allocate(sizeof(Ty) * count));
    }

    bool deallocate(void *p);

    struct Arena {
        void *data;
        size_t capacity;
        size_t occupied;
    };

    Arena make_arena(size_t capacity);
    bool free_arena(Arena *arena);

    #define ARENA_ALLOC_BASIC MAKE_FLAG(0)
    #define ARENA_ALLOC_POPABLE MAKE_FLAG(1)

    void *allocate(Arena *arena, size_t size);

    template <typename Ty>
    inline Ty *
    allocate_struct(Arena *arena)
    {
        return mm::allocate(arena, sizeof(Ty));
    }

    template <typename Ty>
    inline Ty *
    allocate_structs(Arena *arena, uint64_t count)
    {
        return mm::allocate(arena, sizeof(Ty) * count);
    }

    void *arena_alloc(Arena *arena, size_t size, int options);
    void *arena_alloc_set(Arena *arena, size_t size, char c, int options);
    void *arena_alloc_zero(Arena *arena, size_t size, int options);
    bool arena_pop(Arena *arena, void *data);
    size_t arena_reset(Arena *arena);

    struct Stack_View {
        size_t capacity;
        size_t occupied;
        void *data;
    };

    bool can_hold(Stack_View *view, size_t size);

    void *allocate(Stack_View *view, size_t size);

    //
    // @brief Initializes a view in stack-like data-block, and do not own it. Free it yourself!
    //
    Stack_View make_stack_view(void *data, size_t capacity);

    //
    // @brief Allocates new data-block and initializes a view in stack-like data-block, and do not own it. Free it yourself!
    //
    Stack_View make_stack_view(size_t capacity);

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
            uint64_t count;
        } blocks;

        //
        // @brief If greater than zero, tells the allocator to allocate blocks with fixed size, which makes it basiclly
        // behave like pool allocator. If equals zero, blocks will be allocated with specified size aligned to page size.
        //
        size_t block_fixed_size;

        //
        // @brief If greater than zero, sets limit on count of block, which can be allocated. If zero, there will be no
        // limit to block allocation.
        //
        uint64_t block_count_limit;
    };

    Block_Allocator make_block_allocator();

    //
    // @brief Pre-allocates specified amount of blocks with specified size.
    //
    Block_Allocator make_block_allocator(uint64_t blocks_count, size_t block_size, size_t block_fixed_size = 0, uint64_t block_count_limit = 0);

    void *allocate(Block_Allocator *allocator, size_t size);

    template <typename Ty>
    inline Ty *
    allocate_struct(Block_Allocator *allocator)
    {
        return static_cast<Ty *>(mm::allocate(allocator, sizeof(Ty)));
    }

    bool destroy_block_allocator(Block_Allocator *allocator);

} // namespace mm

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

    mm::Arena persist_arena;

    // NOTE(gr3yknigh1): Platform runtime will call issue a draw call if vertexes_count > 0 [2025/03/03]
    mm::Arena vertexes_arena;
    Vertex *vertexes;
    size_t vertexes_count;
};

//
// @param[out] rect Output array of vertexes
//
size_t generate_rect(Vertex *rect, float x, float y, float width, float height, Color4 color);

//
// @param[out] rect Output array of vertexes
//
size_t generate_rect_with_atlas(
    Vertex *rect, float x, float y, float width, float height, Rect_F32 atlas_location, Atlas *altas, Color4 color);

#endif // GARDEN_PLATFORM_H
