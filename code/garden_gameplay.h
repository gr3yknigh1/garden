#if !defined(GARDEN_GAMEPLAY_H)
//
// FILE          code\garden_gameplay.h
//
// AUTHORS
//               Ilya Akkuzin <gr3yknigh1@gmail.com>
//
// NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//
#define GARDEN_GAMEPLAY_H

#include "garden_platform.h"

struct Game_Context;

typedef void *(Game_On_Init_Fn_Type)(Platform_Context *platform);
typedef void (Game_On_Load_Fn_Type)(Platform_Context *platform, Game_Context *game);
typedef void (Game_On_Tick_Fn_Type)(Platform_Context *platform, Game_Context *game, float delta_time);
typedef void (Game_On_Draw_Fn_Type)(Platform_Context *platform, Game_Context *game, float delta_time);
typedef void (Game_On_Fini_Fn_Type)(Platform_Context *platform, Game_Context *game);


#if !defined(GARDEN_GAMEPLAY_DLL_NAME)
    #error "No GARDEN_GAMEPLAY_DLL_NAME was defined during compile!"
#endif

#define GAME_ON_INIT_FN_NAME "game_on_init"
#define GAME_ON_LOAD_FN_NAME "game_on_load"
#define GAME_ON_TICK_FN_NAME "game_on_tick"
#define GAME_ON_DRAW_FN_NAME "game_on_draw"
#define GAME_ON_FINI_FN_NAME "game_on_fini"

#endif // GARDEN_GAMEPLAY_H
