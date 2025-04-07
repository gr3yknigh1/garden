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

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "base/memory.h"

//
// Graphics:
//

typedef int packed_rgba_t;

static_assert(sizeof(packed_rgba_t) == 4);

#if !defined(MAKE_PACKED_RGBA)
    #define MAKE_PACKED_RGBA(R, G, B, A) (((R) << 24) | ((G) << 16) | ((B) << 8) | (A))
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

enum class Camera_ViewMode {
    Perspective,
    Orthogonal
};

struct Camera {
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;

    float yaw;
    float pitch;

    float speed;
    float sensitivity;
    float fov;

    float near;
    float far;

    Camera_ViewMode view_mode;
};

struct Platform_Context {
    Input_State input_state;
    Camera *camera;

    Static_Arena persist_arena;

    // NOTE(gr3yknigh1): Platform runtime will call issue a draw call if vertexes_count > 0 [2025/03/03]
    Static_Arena vertexes_arena;
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
