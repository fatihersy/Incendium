#include "user_interface.h"

#include "core/event.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

Vector2 screen_center = {0};
Vector2 offset = {0};

scene_type gm_current_scene_type = 0;

void user_interface_system_initialize() {

}

void update_user_interface(Vector2 _offset, Vector2 _screen_half_size, scene_type _current_scene_type, Camera2D _camera) {
    screen_center = _screen_half_size;

    offset = _offset;
    gm_current_scene_type = _current_scene_type;
}

void render_user_interface() {
    switch (gm_current_scene_type) {
        case scene_main_menu: 
        {
            if (GuiButton((Rectangle){screen_center.x - BTN_DIM_X_DIV2, screen_center.y - BTN_DIM_Y_DIV2 - 40, BTN_DIM_X, BTN_DIM_Y}, "Play"))
            {
                
            };
            if (GuiButton((Rectangle){screen_center.x - BTN_DIM_X_DIV2, screen_center.y - BTN_DIM_Y_DIV2, BTN_DIM_X, BTN_DIM_Y}, "Settings")) 
            {
                // TODO: Settings
            };
            if (GuiButton((Rectangle){screen_center.x - BTN_DIM_X_DIV2, screen_center.y - BTN_DIM_Y_DIV2 + 40, BTN_DIM_X, BTN_DIM_Y}, "Quit")) 
            {
                event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context){0});
            };

            break;
        }
        case scene_in_game: {
            DrawFPS(offset.x, offset.y);
            break;
        }

        default:
            break;
    }
}
