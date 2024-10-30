#include "game.h"


int main() 
{
    if(!game_initialize()) return 1;

    while (!WindowShouldClose())
    {
        game_update();

        game_render();
    }

    // TODO: Destroy game

    return 0;
}