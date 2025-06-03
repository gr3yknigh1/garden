

#include <math.h>   // sqrtf, powf

#include "garden_runtime.h"
#include "garden_gameplay.h"

struct Game_Context {
    Float32 player_x, player_y, player_w, player_h;
    Float32 player_speed;

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
    Game_Context *game = mm::allocate_struct<Game_Context>(&platform->persist_arena, ALLOCATE_NO_OPTS);

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
    game->player_speed = 100;
}

extern "C" __declspec(dllexport) void
game_on_tick(Platform_Context *platform, Game_Context *game, float delta_time)
{
    Float32 x_direction = 0, y_direction = 0;

    if (is_key_down(&platform->input_state, Key_Code::Left)) {
        x_direction = -1;
    }

    if (is_key_down(&platform->input_state, Key_Code::Right)) {
        x_direction = +1;
    }

    if (is_key_down(&platform->input_state, Key_Code::Down)) {
        y_direction = -1;
    }

    if (is_key_down(&platform->input_state, Key_Code::Up)) {
        y_direction = +1;
    }

    if (absolute(x_direction) >= 1.0f && absolute(y_direction) >= 1.0f) {
        // TODO(gr3yknigh1): Fix strange floating-point bug for diagonal movement [2025/02/20]
        normalize_vector2f(&x_direction, &y_direction);
    }

    game->player_x += game->player_speed * x_direction * delta_time;
    game->player_y += game->player_speed * y_direction * delta_time;


    platform->camera->position.x = -game->player_x - game->player_w / 2;
    platform->camera->position.y = -game->player_y - game->player_h / 2;
}


extern "C" __declspec(dllexport) void
game_on_draw(Platform_Context *platform, Game_Context *game, [[maybe_unused]] float delta_time)
{
    // XXX
    Color4 rect_color = { 255, 255, 255, 255  };
    Atlas atlas = { 32, 32 };

    platform->vertexes = mm::allocate_structs<Vertex>(&platform->vertexes_arena, 6);
    platform->vertexes_count = generate_rect_with_atlas(platform->vertexes, game->player_x, game->player_y, game->player_w, game->player_h, game->atlas_location, &atlas, rect_color);


    #if 0
    static bool show_debug_panel = true;
    ImGui::Begin("Debug panel", &show_debug_panel, 0);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::EndMenu();
        }

        HelpMarker("Hi!"
                   "\n"
                   "Dev version.");

        ImGui::EndMenuBar();
    }

    if (ImGui::CollapsingHeader("Debug Info")) {
    }

    ImGui::End();

    #endif
}

extern "C" __declspec(dllexport) void
game_on_fini([[maybe_unused]] Platform_Context *platform, [[maybe_unused]] Game_Context *game)
{
}


Float32
absolute(Float32 x)
{
    return x < 0 ? -x : x;
}

void
normalize_vector2f(Float32 *x, Float32 *y)
{
    Float32 magnitude = sqrtf(powf(*x, 2) + powf(*y, 2));
    *x /= magnitude;
    *y /= magnitude;
}
