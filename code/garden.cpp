

#include <math.h>   // sqrtf, powf

#include <windows.h>

#include "garden_platform.h"
#include "garden_gameplay.h"


struct Game_Context {
    float player_x, player_y;
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
    Game_Context *game = static_cast<Game_Context *>(arena_alloc_zero(&platform->persist_arena, sizeof(game), 0));

    game->player_x = 20;
    game->player_y = 20;
    game->player_speed = 400;
    game->atlas_location = { 0, 0, 16, 16 };

    return game;
}

extern "C" __declspec(dllexport) void
game_on_load(Platform_Context *platform, Game_Context *game)
{
    game->player_speed = 300;
}

extern "C" __declspec(dllexport) void
game_on_tick(Platform_Context *platform, Game_Context *game, float delta_time)
{
    game->player_x += game->player_speed * platform->input_state.x_direction * delta_time;
    game->player_y += game->player_speed * platform->input_state.y_direction * delta_time;


    if (absolute(platform->input_state.x_direction) >= 1.0f && absolute(platform->input_state.y_direction) >= 1.0f) {
        // TODO(gr3yknigh1): Fix strange floating-point bug for diagonal movement [2025/02/20]
        normalize_vector2f(&platform->input_state.x_direction, &platform->input_state.y_direction);
    }
}

extern "C" __declspec(dllexport) void
game_on_draw(Platform_Context *platform, Game_Context *game, float delta_time)
{
    (void)delta_time;

    // XXX
    static Color4 rect_color = { 200, 100, 0, 255 };
    static Atlas atlas = { 32, 32 };

    platform->vertexes = static_cast<Vertex *>(arena_alloc_zero(&platform->vertexes_arena, sizeof(Vertex) * 6, ARENA_ALLOC_BASIC));
    // platform->vertexes_count = generate_rect(vertexes, player_x, player_y, 100, 100, rect_color);
    platform->vertexes_count = generate_rect_with_atlas(platform->vertexes, game->player_x, game->player_y, 100, 100, game->atlas_location, &atlas, rect_color);
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