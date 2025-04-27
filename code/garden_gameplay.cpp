

#include <math.h>   // sqrtf, powf

#include "garden_runtime.h"
#include "garden_gameplay.h"

struct Game_Context {
    F32 player_x, player_y, player_w, player_h;
    F32 player_speed;

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
    Game_Context *game = static_cast<Game_Context *>(allocate(&platform->persist_arena, sizeof(game), 0));

    game->player_x = 20;
    game->player_y = 20;
    game->player_w = 100;
    game->player_h = 100;
    game->player_speed = 400;
    game->atlas_location = { 0, 0, 16, 16 };

    return game;
}


extern "C" __declspec(dllexport) void
game_on_load([[maybe_unused]] Platform_Context *platform, Game_Context *game)
{
    Linked_List<int> nums{};
    nums.push_back(1);
    nums.push_back(2);
    nums.push_back(3);

    for (Linked_List<int>::Forward_Iterator it = nums.begin(); it != nums.rend(); ++it) {
        printf("%d ", *it);
    }

    game->player_speed = 1000;
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
game_on_draw(Platform_Context *platform, Game_Context *game, [[maybe_unused]] float delta_time)
{
    // XXX
    Color4 rect_color = { 255, 255, 255, 255  };
    Atlas atlas = { 32, 32 };

    platform->vertexes = allocate_structs<Vertex>(&platform->vertexes_arena, 6);
    platform->vertexes_count = generate_rect_with_atlas(platform->vertexes, game->player_x, game->player_y, game->player_w, game->player_h, game->atlas_location, &atlas, rect_color);
}

extern "C" __declspec(dllexport) void
game_on_fini([[maybe_unused]] Platform_Context *platform, [[maybe_unused]] Game_Context *game)
{
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
