#include "game_manager.h"

#include "core/event.h"
#include "core/fmath.h"
#include "core/ftime.h"

#include "defines.h"
#include "player.h"
#include "raylib.h"
#include "resource.h"
#include "spawn.h"


#include "user_interface.h"

const int gridsize = 33;
const int map_size = gridsize * 100;
Vector2 screen_size = {0};
Vector2 screen_half_size = {0};

scene_type current_scene_type = 0;

bool game_manager_initialized = false;
bool is_game_paused = false;

void draw_background();
void show_pause_screen();
void clean_up();

bool game_manager_on_event(u16 code, void *sender, void *listener_inst,
                           event_context context);

bool game_manager_initialize(Vector2 _screen_size) {
  if (game_manager_initialized)
    return false;

  if (!resource_system_initialize())
    return false;
  if (!player_system_initialize())
    return false;
  if (!spawn_system_initialize())
    return false;
  user_interface_system_initialize();

  screen_size = _screen_size;
  screen_half_size = (Vector2){_screen_size.x / 2, _screen_size.y / 2};

  current_scene_type = SCENE_MAIN_MENU;
  load_scene();

  game_manager_initialized = true;

  event_register(EVENT_CODE_SCENE_IN_GAME, 0, game_manager_on_event);
  event_register(EVENT_CODE_PAUSE_GAME, 0, game_manager_on_event);
  event_register(EVENT_CODE_UNPAUSE_GAME, 0, game_manager_on_event);
  event_register(EVENT_CODE_RETURN_MAIN_MENU_GAME, 0, game_manager_on_event);

  return true;
}

void set_player_position(i16 x, i16 y) {
  event_fire(EVENT_CODE_PLAYER_SET_POSITION, 0, (event_context) 
  { 
    .data.f32[0] = x,
    .data.f32[1] = y,
  });
}

void set_current_scene_type(scene_type type) { current_scene_type = type; }
Vector2 get_player_position() { return get_player_state()->position;}
Character2D *get_actor_by_id(u16 ID) {
  spawn_system_state *spawn_data = get_spawn_system();

  for (i32 i = 0; i < MAX_SPAWN_COUNT; i++) {
    if (spawn_data->spawns[i].character_id == ID)
      return &spawn_data->spawns[i];
  }

  return (Character2D *){0};
}
float get_time_elapsed(elapse_time_type type) {
  timer *time = get_timer();

  if (time->remaining == 0) {
    reset_time(type);
    return 0;
  };

  return time->remaining;
}
scene_type get_current_scene_type() { return current_scene_type; }

Vector2 get_player_dimentions() {
  return get_player_state()->dimentions;
}

void update_game_manager() {

  if(GetFPS() > TARGET_FPS) return;
  //TraceLog(LOG_INFO, "player.position {x:%d, y:%d}", get_player_position().x, get_player_position().y);

  switch (current_scene_type) {
  case SCENE_MAIN_MENU: {
      update_resource_system();
    break;
  }
  case SCENE_IN_GAME: {
    if (IsKeyPressed(KEY_ESCAPE)) {
      is_game_paused = !is_game_paused;
        (is_game_paused) 
        ? event_fire(EVENT_CODE_PAUSE_GAME, 0, (event_context){0})
        : event_fire(EVENT_CODE_UNPAUSE_GAME, 0, (event_context){0});
      break;
    }

    if(!is_game_paused) {
      update_player();
      update_spawns(get_player_position());
      update_resource_system();
    }

    break;
  }

  default:
    break;
  }
}

void render_game_manager() {
  
  if(GetFPS() > TARGET_FPS) return;
  draw_background();    


  switch (current_scene_type) {
  case SCENE_MAIN_MENU: {
    render_resource_system();
    break;
  }
  case SCENE_IN_GAME: {
    render_player();
    render_spawns();
    render_resource_system();
    break;
  }
  default:
    break;
  }
}

void load_scene() {
  set_player_position(screen_size.x / 2, screen_size.y / 2);

  switch (current_scene_type) {
  case SCENE_MAIN_MENU: {
    break;
  }
  case SCENE_IN_GAME: {
    for (u32 i = 0; i < 360; i += 20) {
      Vector2 position = get_a_point_of_a_circle(get_player_position(), 500, i);

      spawn_character(
          (Character2D){
              .res_type = ENEMY_TEXTURE,
              .initialized = true,
              .position = position,
              .speed = 1,
              .health = 100,
              .damage = 10,
          },
          ENEMY);
    }

    break;
  }

  default:
    break;
  }
}

bool damage_any_collade(Character2D *_character) {
  spawn_system_state *spawn_system = get_spawn_system();

  for (u32 i = 0; i < spawn_system->current_spawn_count; i++) {
    if (CheckCollisionRecs(spawn_system->spawns[i].collision_rect,
                           _character->collision_rect)) {
      kill_spawn(spawn_system->spawns[i].character_id);
      return true;
    }
  }

  return false;
}

void draw_background() {
  switch (current_scene_type) {
  case SCENE_MAIN_MENU: {
    //Centering guidelines
/*     DrawLine(screen_size.x / 2, 0, screen_size.x / 2, screen_size.y,
    (Color){255, 255, 255, 255}); DrawLine(0, screen_size.y / 2,
    screen_size.x, screen_size.y / 2, (Color){255, 255, 255, 255}); */
      DrawTexturePro(
      get_texture_by_enum(BACKGROUND),
        (Rectangle){.x = 0,
                    .y = 0,
                    .width = get_texture_by_enum(BACKGROUND).width,
                    .height = get_texture_by_enum(BACKGROUND).height},
        (Rectangle){.x = 0,
                    .y = 0,
                    .width = GetScreenWidth(),
                    .height = GetScreenHeight()},
        (Vector2){.x = 0, .y = 0}, 0,
        WHITE); // Draws the background to main menu
    break;
  }
  case SCENE_IN_GAME: {
    for (i16 i = -map_size + 13; i < map_size; i += gridsize) { // X Axis
      DrawLine(i, -map_size, i, i + (map_size), (Color){21, 17, 71, 255});
    }
    for (i16 i = -map_size - 3; i < map_size; i += gridsize) { // Y Axis
      DrawLine(-map_size, i, i + (map_size), i, (Color){21, 17, 71, 255});
    }
    break;
  }

  default:
    break;
  }
}

bool game_manager_on_event(u16 code, void *sender, void *listener_inst,
                           event_context context) {
  switch (code) {
  case EVENT_CODE_SCENE_IN_GAME: {
    current_scene_type = SCENE_IN_GAME;
    load_scene();
    return true;
    break;
  }
  case EVENT_CODE_SCENE_MAIN_MENU: {
    current_scene_type = SCENE_MAIN_MENU;
    load_scene();
    return true;
    break;
  }
  case EVENT_CODE_PAUSE_GAME: {
    is_game_paused = true;
    event_fire(EVENT_CODE_UI_SHOW_PAUSE_SCREEN, 0, (event_context){0});

    return true;
    break;
  }
  case EVENT_CODE_UNPAUSE_GAME: {
    is_game_paused = false;
    event_fire(EVENT_CODE_UI_SHOW_UNPAUSE_SCREEN, 0, (event_context){0});

    return true;
    break;
  }
  case EVENT_CODE_RETURN_MAIN_MENU_GAME: {
    is_game_paused = false;
    current_scene_type = SCENE_MAIN_MENU;
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

void clean_up() { clean_up_spawn_system(); }
