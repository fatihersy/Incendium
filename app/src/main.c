#include "app.h"


int main(void) 
{
    if(!app_initialize()) return 1;

    while (window_should_close())
    {
        app_update();

        app_render();
    }

    // TODO: Destroy game

    return 0;
}
