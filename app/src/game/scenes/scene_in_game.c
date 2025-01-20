#include "scene_in_game.h"
#include <defines.h>
#include <settings.h>

#include <core/event.h>
#include <core/fmath.h>
#include <core/fmemory.h>

#include "game/resource.h"
#include "game/game_manager.h"
#include "game/user_interface.h"

typedef struct scene_in_game_state {
  game_manager_system_state *p_game_manager;
  player_state* player;
  panel skill_up_panels[MAX_UPDATE_ABILITY_PANEL_COUNT];
  

} scene_in_game_state;

static scene_in_game_state *state;

#define STATE_ASSERT(FUNCTION) if (!state) {                                                          \
    TraceLog(LOG_ERROR, "scene_in_game::" FUNCTION "::In game state was not initialized");            \
    event_fire(EVENT_CODE_SCENE_MAIN_MENU, 0, (event_context){});                                     \
    return;                                                                                           \
  }

void in_game_update_bindings();
void in_game_update_mouse_bindings();
void in_game_update_keyboard_bindings();

bool initialize_scene_in_game(Vector2 _screen_size) {

  state = (scene_in_game_state *)allocate_memory_linear(sizeof(scene_in_game_state), true);

  // Game
  if (!game_manager_initialize(_screen_size, SCENE_IN_GAME)) { // Inits player & spawns
    TraceLog(LOG_ERROR, "game_manager_initialize() failed");
    return false;
  }
  state->p_game_manager = get_game_manager();
  state->player = get_player_state_if_available();

  _set_player_position(*get_resolution_div2());

  user_interface_system_initialize();

  for (u32 i = 0; i < 360; i += 20) {
    Vector2 position = get_a_point_of_a_circle(_get_player_position(false), 500, i);
    Texture2D *tex = get_texture_by_enum(TEX_ID_ENEMY_TEXTURE);
    rectangle_collision rect_col = (rectangle_collision) {
      .rect = (Rectangle) {
        .x = position.x,
        .y = position.y,
        .width = tex->width,
        .height = tex->height
      },
      .owner_type = ENEMY
    };
    rect_col.owner_id = _spawn_character((Character2D){
        .character_id = 0,
        .tex = tex,
        .initialized = false,
        .collision = rect_col.rect,
        .position = position,
        .w_direction = LEFT,
        .type = ENEMY,
        .rotation = 0,
        .health = 100,
        .damage = 10,
        .speed = 1,
    });
    state->p_game_manager->spawn_collision_count++;
    state->p_game_manager->spawn_collisions[state->p_game_manager->spawn_collision_count] = rect_col;
    state->p_game_manager->spawn_collisions[state->p_game_manager->spawn_collision_count].is_active = true;
  }

  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, 0, (event_context){
    .data.f32[0] = PRG_BAR_ID_PLAYER_EXPERIANCE,
    .data.f32[1] = get_player_state_if_available()->exp_perc,
  });
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, 0, (event_context){
    .data.f32[0] = PRG_BAR_ID_PLAYER_HEALTH,
    .data.f32[1] = get_player_state_if_available()->health_perc,
  });
  panel default_panel = (panel) {
    .signal_state  = BTN_STATE_RELEASED,
    .bg_tex_id     = TEX_ID_CRIMSON_FANTASY_PANEL_BG,
    .frame_tex_id  = TEX_ID_CRIMSON_FANTASY_PANEL,
    .bg_tint       = (Color) {255, 255, 255, 200},
    .bg_hover_tint = (Color) {255, 55, 55, 184},
    .offsets = (Vector4) {6, 6, 6, 6},
  };
  for (int i=0; i<MAX_UPDATE_ABILITY_PANEL_COUNT; ++i) {
    state->skill_up_panels[i] = default_panel;
  }

  state->p_game_manager->is_game_paused = false;

  return true;
}

void update_scene_in_game() {
  STATE_ASSERT("update_scene_in_game")

  update_user_interface();
  in_game_update_bindings();

  if (state->p_game_manager->is_game_paused) {
    return;
  }

  _update_player();
  _update_spawns();
  update_game_manager();

  event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, (event_context){
    .data.f32[0] = _get_player_position(false).x,
    .data.f32[1] = _get_player_position(false).y,
  });
}

void render_scene_in_game() {
  STATE_ASSERT("render_scene_in_game")

  _render_map();

  _render_player();
  _render_spawns();
}

void render_interface_in_game() {
  STATE_ASSERT("render_interface_in_game")

  if (state->player->is_player_have_skill_points) {
    state->p_game_manager->is_game_paused = true;
    Rectangle dest = (Rectangle) { // TODO: Make it responsive
      get_resolution_div4()->x, get_resolution_div2()->y, 
      get_resolution_div4()->x, get_resolution_3div2()->y 
    };
    f32 dest_x_buffer = dest.x;
    for (int i=0; i<MAX_UPDATE_ABILITY_PANEL_COUNT; ++i) {
      dest.x = dest_x_buffer + ((dest.width + get_screen_offset()) * i);
      if(gui_panel_clickable(&state->skill_up_panels[i], dest, true)) {
        state->p_game_manager->is_game_paused = false;
        state->player->is_player_have_skill_points = false;


      }
    }


  }
  else {
    gui_progress_bar(PRG_BAR_ID_PLAYER_EXPERIANCE, (Vector2){.x = get_resolution_div2()->x, .y = get_screen_offset()}, true);
    gui_progress_bar(PRG_BAR_ID_PLAYER_HEALTH, (Vector2){.x = get_screen_offset(), .y = get_screen_offset()}, false);
  }

  render_user_interface();
}


void in_game_update_bindings() {
  in_game_update_mouse_bindings();
  in_game_update_keyboard_bindings();
}

void in_game_update_mouse_bindings() { 

}
void in_game_update_keyboard_bindings() {

  if (IsKeyReleased(KEY_ESCAPE)) {
    if(!state->player->is_player_have_skill_points) state->p_game_manager->is_game_paused = !state->p_game_manager->is_game_paused;
    event_fire(EVENT_CODE_UI_SHOW_PAUSE_MENU, 0, (event_context){0});
  }

}




