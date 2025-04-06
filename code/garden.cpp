

#include <math.h>   // sqrtf, powf

#if !defined(UNICODE)
    #define UNICODE
#endif

#if !defined(NOMINMAX)
    #define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#if defined(far)
    #undef far
#endif

#if defined(near)
    #undef near
#endif

#include "garden_platform.h"
#include "garden_gameplay.h"


struct Game_Context {
    float player_x, player_y, player_w, player_h;
    float player_speed;

    Rect_F32 atlas_location;
};


//
// Linear:
//

float absolute(float x);
void normalize_vector2f(float *x, float *y);


extern "C" __declspec(dllexport) void *
game_on_init(Platform_Context *platform)
{
    Game_Context *game = static_cast<Game_Context *>(mm::arena_alloc_zero(&platform->persist_arena, sizeof(game), 0));

    game->player_x = 20;
    game->player_y = 20;
    game->player_w = 100;
    game->player_h = 100;
    game->player_speed = 400;
    game->atlas_location = { 0, 0, 16, 16 };

    return game;
}

extern "C" __declspec(dllexport) void
game_on_load(Platform_Context *platform, Game_Context *game)
{
    (void)platform;

    game->player_speed = 300;
}

extern "C" __declspec(dllexport) void
game_on_tick(Platform_Context *platform, Game_Context *game, float delta_time)
{
    if (absolute(platform->input_state.x_direction) >= 1.0f && absolute(platform->input_state.y_direction) >= 1.0f) {
        // TODO(gr3yknigh1): Fix strange floating-point bug for diagonal movement [2025/02/20]
        normalize_vector2f(&platform->input_state.x_direction, &platform->input_state.y_direction);
    }

    game->player_x += game->player_speed * platform->input_state.x_direction * delta_time;
    game->player_y += game->player_speed * platform->input_state.y_direction * delta_time;


    platform->camera->position.x = -game->player_x - game->player_w / 2;
    platform->camera->position.y = -game->player_y - game->player_h / 2;
}

extern "C" __declspec(dllexport) void
game_on_draw(Platform_Context *platform, Game_Context *game, float delta_time)
{
    (void)delta_time;

    // XXX
    Color4 rect_color = { 255, 255, 255, 255  };
    Atlas atlas = { 32, 32 };

    platform->vertexes = static_cast<Vertex *>(arena_alloc_zero(&platform->vertexes_arena, sizeof(Vertex) * 6, ARENA_ALLOC_BASIC));
    platform->vertexes_count = generate_rect_with_atlas(platform->vertexes, game->player_x, game->player_y, game->player_w, game->player_h, game->atlas_location, &atlas, rect_color);
}

extern "C" __declspec(dllexport) void
game_on_fini(Platform_Context *platform, Game_Context *game)
{
    (void)platform;
    (void)game;
}


float
absolute(float x)
{
    return x < 0 ? -x : x;
}

void
normalize_vector2f(float *x, float *y)
{
    float magnitude = sqrtf(powf(*x, 2) + powf(*y, 2));
    *x /= magnitude;
    *y /= magnitude;
}

//
// TODO(gr3yknigh1): Move this out to separate platform dependent module [2025/03/03]
//

extern "C" BOOL WINAPI
DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void)instance;
    (void)reserved;

    switch(reason) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
