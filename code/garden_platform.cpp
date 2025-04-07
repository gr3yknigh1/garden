//
// FILE          code\garden_platform.cpp
//
// AUTHORS
//               Ilya Akkuzin <gr3yknigh1@gmail.com>
//
// NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//

#include <assert.h>

#include <memory.h>

#if 0 && defined(GARDEN_USE_CRT_ALLOCATIONS)
    // TODO(gr3yknig1): Do something similiar [2025/04/07]
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
