
#include "topdown_platform.hpp"


struct Game_Context {


};

extern "C" void *
game_on_init(void)
{
    static Game_Context context;

    return &context;
}

extern "C" void
game_on_tick(Game_Context *context, float delta_time)
{

}
