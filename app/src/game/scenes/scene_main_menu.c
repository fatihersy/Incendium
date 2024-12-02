
#include "scene_main_menu.h"

#include "defines.h"
#include "game/camera.h"
#include "game/game_manager.h"
#include "game/scenes/scene_manager.h"
#include "game/user_interface.h"
#include "raylib.h"

void initialize_scene_main_menu() {  // Game
  if (!game_manager_initialize((Vector2) {SCREEN_WIDTH, SCREEN_HEIGHT})) {
    TraceLog(LOG_ERROR, "game_manager_initialize() failed");
    return;
  }

  user_interface_system_initialize(); // Requires player

}

void update_scene_main_menu() {
    update_game_manager(SCENE_MAIN_MENU);
    update_user_interface((Vector2){5, 5}, get_game_manager()->screen_half_size, get_current_scene_type(), get_active_camera());
}

void render_scene_main_menu() {
    render_user_interface();
}


