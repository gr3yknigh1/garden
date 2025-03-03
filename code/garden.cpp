#include <stdio.h>

#include <windows.h>

#include "garden_platform.h"
#include "garden_gameplay.h"

struct Game_Context {
    int mock;
};

extern "C" __declspec(dllexport) void *
game_on_init(Platform_Context *platform)
{
    (void)platform;

    static Game_Context context;

    return &context;
}

extern "C" __declspec(dllexport) void
game_on_tick(Platform_Context *platform, Game_Context *game, float delta_time)
{
    (void)platform;
    (void)game;
    (void)delta_time;
}

extern "C" __declspec(dllexport) void
game_on_draw(Platform_Context *platform, Game_Context *game, float delta_time)
{
    (void)platform;
    (void)game;
    (void)delta_time;
}

extern "C" __declspec(dllexport) void
game_on_fini(Platform_Context *platform, Game_Context *game)
{
    (void)platform;
    (void)game;
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