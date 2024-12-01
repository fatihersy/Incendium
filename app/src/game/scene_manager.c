#include "scene_manager.h"
#include "defines.h"

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"

#include "player.h"
#include "raylib.h"
#include "resource.h"
#include "spawn.h"

void clean_up();
void draw_background();

static scene_manager_system_state *scene_manager_state;

bool scene_manager_initialized = false;
bool scene_manager_on_event(u16 code, void *sender, void *listener_inst,
                            event_context context);

bool scene_manager_initialize(game_manager_system_state *_game_manager) {
  if (scene_manager_initialized)
    return false;

  scene_manager_state = (scene_manager_system_state *)allocate_memory_linear(
      sizeof(scene_manager_system_state), true);

  scene_manager_state->p_game_manager = _game_manager;

  if (!player_system_initialize()) {
    return false;
  }
  if (!spawn_system_initialize()) {
    return false;
  }

  scene_manager_state->gridsize = 33;
  scene_manager_state->map_size = scene_manager_state->gridsize * 100;
  scene_manager_state->current_scene_type = 0;

  event_register(EVENT_CODE_SCENE_IN_GAME, 0, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_IN_GAME_EDIT, 0, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_MAIN_MENU, 0, scene_manager_on_event);
  event_register(EVENT_CODE_RETURN_MAIN_MENU_GAME, 0, scene_manager_on_event);

  return true;
}

void update_scene_manager() {}
void update_scene_main_menu() {}

void update_scene_in_game() {
  game_manager_system_state *game_manager_state =
      scene_manager_state->p_game_manager;

  if (scene_manager_state->current_scene_type == SCENE_IN_GAME) {
    if (IsKeyPressed(KEY_ESCAPE)) {
      game_manager_state->is_game_paused = !game_manager_state->is_game_paused;
      (game_manager_state->is_game_paused)
          ? event_fire(EVENT_CODE_PAUSE_GAME, 0, (event_context){0})
          : event_fire(EVENT_CODE_UNPAUSE_GAME, 0, (event_context){0});
    }
    if (!game_manager_state->is_game_paused) {
      update_player();
      update_spawns(get_player_state()->position);
    }
  }
}

void render_scene_main_menu() {}
void render_scene_in_game() {
  draw_background();

  render_player();
  render_spawns();
}
void update_scene_in_game_edit() {

  if (IsKeyDown(KEY_W)) {
    scene_manager_state->spectator_location.y -= 2;
  }
  if (IsKeyDown(KEY_A)) {
    scene_manager_state->spectator_location.x -= 2;
  }
  if (IsKeyDown(KEY_S)) {
    scene_manager_state->spectator_location.y += 2;
  }
  if (IsKeyDown(KEY_D)) {
    scene_manager_state->spectator_location.x += 2;
  }
}

void render_scene_in_game_edit() { draw_background(); }

void load_scene() {
  game_manager_system_state *game_manager_state =
      scene_manager_state->p_game_manager;

  switch (scene_manager_state->current_scene_type) {
  case SCENE_MAIN_MENU: {
    break;
  }
  case SCENE_IN_GAME: {
    for (u32 i = 0; i < 360; i += 20) {

      set_player_position(game_manager_state->screen_size.x / 2,
                          game_manager_state->screen_size.y / 2);

      Vector2 position =
          get_a_point_of_a_circle(get_player_position(false), 500, i);
      Texture2D *tex = get_texture_by_enum(ENEMY_TEXTURE);
      rectangle_collision rect_col =
          (rectangle_collision){.rect = (Rectangle){.x = position.x,
                                                    .y = position.y,
                                                    .width = tex->width,
                                                    .height = tex->height},
                                .owner_type = ENEMY};
      rect_col.owner_id = spawn_character((Character2D){
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
      game_manager_state->spawn_collision_count++;
      game_manager_state
          ->spawn_collisions[game_manager_state->spawn_collision_count] =
          rect_col;
      game_manager_state
          ->spawn_collisions[game_manager_state->spawn_collision_count]
          .is_active = true;
    }
    break;
  }
  case SCENE_IN_GAME_EDIT: {

    break;
  }

  default:
    break;
  }
}
bool scene_manager_on_event(u16 code, void *sender, void *listener_inst,
                            event_context context) {
  switch (code) {
  case EVENT_CODE_SCENE_IN_GAME: {
    scene_manager_state->current_scene_type = SCENE_IN_GAME;
    load_scene();
    return true;
    break;
  }
  case EVENT_CODE_SCENE_IN_GAME_EDIT: {
    scene_manager_state->current_scene_type = SCENE_IN_GAME_EDIT;
    load_scene();
    return true;
    break;
  }
  case EVENT_CODE_SCENE_MAIN_MENU: {
    scene_manager_state->current_scene_type = SCENE_MAIN_MENU;
    load_scene();
    return true;
    break;
  }
  case EVENT_CODE_RETURN_MAIN_MENU_GAME: {
    scene_manager_state->current_scene_type = SCENE_MAIN_MENU;
    clean_up();
    load_scene();

    return true;
    break;
  }
  default:
    break;
  }

  return false;
}
void draw_background() {
  u16 ms = scene_manager_state->map_size;

  switch (scene_manager_state->current_scene_type) {
  case SCENE_MAIN_MENU: {
    break;
  }
  case SCENE_IN_GAME: {
    for (i16 i = -ms; i < ms; i += 16) { // X Axis
      DrawLine(i, -ms, i, i + (ms), (Color){21, 17, 71, 255});
    }
    for (i16 i = -ms; i < ms; i += 16) { // Y Axis
      DrawLine(-ms, i, i + (ms), i, (Color){21, 17, 71, 255});
    }
    break;
  }
  case SCENE_IN_GAME_EDIT: {
    for (i16 i = -ms; i < ms; i += 16) { // X Axis
      DrawLine(i, -ms, i, i + (ms), (Color){255, 255, 255, 255});
    }
    for (i16 i = -ms; i < ms; i += 16) { // Y Axis
      DrawLine(-ms, i, i + (ms), i, (Color){255, 255, 255, 255});
    }
    break;
  }

  default:
    break;
  }
}

void set_player_position(i16 x, i16 y) {
  event_fire(EVENT_CODE_PLAYER_SET_POSITION, 0,
             (event_context){
                 .data.f32[0] = x,
                 .data.f32[1] = y,
             });
}

void set_current_scene_type(scene_type type) {
  scene_manager_state->current_scene_type = type;
}

Vector2 get_player_position(bool centered) {
  return centered 
  ? (Vector2) {
        .x = get_player_state()->position.x - get_player_state()->dimentions_div2.x,
        .y = get_player_state()->position.y - get_player_state()->dimentions_div2.y,}
  : get_player_state()->position;
}

Character2D *get_actor_by_id(u16 ID) {
  spawn_system_state *spawn_data = get_spawn_system();

  for (i32 i = 0; i < MAX_SPAWN_COUNT; i++) {
    if (spawn_data->spawns[i].character_id == ID)
      return &spawn_data->spawns[i];
  }

  return (Character2D *){0};
}

scene_type get_current_scene_type() {
  return scene_manager_state->current_scene_type;
}

Vector2 get_player_dimentions() { return get_player_state()->dimentions; }
Vector2 get_spectator_position() {
  return scene_manager_state->spectator_location;
}

void clean_up() { clean_up_spawn_system(); }