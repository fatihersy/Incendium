#include "game.h"


int main() 
{
    game_initialize();

    while (!WindowShouldClose())
    {
        game_update();

        game_render();
    }

    // TODO: Destroy game

    return true;
}