#include "app.h"


int main() 
{
    if(!app_initialize()) return 1;

    while (!WindowShouldClose())
    {
        app_update();

        app_render();
    }

    // TODO: Destroy game

    return 0;
}