#include "game_manager.h"

#include "core/event.h"
#include "core/fmemory.h"
#include "core/ftime.h"

#include "game/player.h"
#include "game/spawn.h"
#include "game/scene_manager.h"

static game_manager_system_state *game_manager_state;

bool game_manager_initialized = false;
bool game_manager_on_event(u16 code, void *sender, void *listener_inst, event_context context);

bool game_manager_initialize(Vector2 _screen_size) {
  if (game_manager_initialized)
    return false;

  game_manager_state = (game_manager_system_state *)allocate_memory_linear(
      sizeof(game_manager_system_state), true);

  game_manager_state->screen_size = (Vector2){0};
  game_manager_state->screen_half_size = (Vector2){0};
  game_manager_state->is_game_paused = false;
  game_manager_initialized = true;

  game_manager_state->screen_size = _screen_size;
  game_manager_state->screen_half_size =
      (Vector2){_screen_size.x / 2, _screen_size.y / 2};

  if (!scene_manager_initialize(game_manager_state)) {
    return false;
  }
  set_current_scene_type(SCENE_MAIN_MENU);

  event_register(EVENT_CODE_PAUSE_GAME, 0, game_manager_on_event);
  event_register(EVENT_CODE_UNPAUSE_GAME, 0, game_manager_on_event);
  event_register(EVENT_CODE_RELOCATE_SPAWN_COLLISION, 0, game_manager_on_event);

  return true;
}

void update_game_manager() {
  if (GetFPS() > TARGET_FPS) return;
 
  update_scene_manager();

  switch (get_current_scene_type()) {
    case SCENE_IN_GAME: update_scene_in_game(); break;
    case SCENE_MAIN_MENU: update_scene_main_menu();  break;
    case SCENE_IN_GAME_EDIT: update_scene_in_game_edit(); break;
  default: break;
  }
}

void render_game_manager() {
  if (GetFPS() > TARGET_FPS) return;

  switch (get_current_scene_type()) {
  case SCENE_MAIN_MENU: { render_scene_main_menu(); break;}
  case SCENE_IN_GAME: { render_scene_in_game(); break;}
  case SCENE_IN_GAME_EDIT: { render_scene_in_game_edit(); break;}
  default: break;
  }
}

bool game_manager_on_event(u16 code, void *sender, void *listener_inst,
                           event_context context) {
  switch (code) {
  case EVENT_CODE_PAUSE_GAME: {
    game_manager_state->is_game_paused = true;
    event_fire(EVENT_CODE_UI_SHOW_PAUSE_SCREEN, 0, (event_context){0});

    return true;
    break;
  }
  case EVENT_CODE_UNPAUSE_GAME: {
    game_manager_state->is_game_paused = false;
    event_fire(EVENT_CODE_UI_SHOW_UNPAUSE_SCREEN, 0, (event_context){0});

    return true;
    break;
  }

  case EVENT_CODE_RELOCATE_SPAWN_COLLISION: {
    for (int i = 1; i <= game_manager_state->spawn_collision_count; ++i) {
      if (game_manager_state->spawn_collisions[i].owner_id ==
          context.data.u16[0]) {
        game_manager_state->spawn_collisions[i].rect.x = context.data.u16[1];
        game_manager_state->spawn_collisions[i].rect.y = context.data.u16[2];
        return true;
      }
    }
    break;
  }
  default:
    break;
  }

  return false;
}

float get_time_elapsed(elapse_time_type type) {
  timer *time = get_timer();

  if (time->remaining == 0) {
    reset_time(type);
    return 0;
  };

  return time->remaining;
}

void damage_any_spawn(Character2D *projectile) {
  if (!projectile) TraceLog(LOG_ERROR, 
  "ERROR::game_manager::check_any_collide()::Tried to check collide with uninitialized actor");
  
  for (u16 i = 1; i <= game_manager_state->spawn_collision_count; ++i) {
    if (game_manager_state->spawn_collisions[i].is_active){
      if (CheckCollisionRecs(game_manager_state->spawn_collisions[i].rect, projectile->collision)) {
        u16 result = damage_spawn(game_manager_state->spawn_collisions[i].owner_id, projectile->damage);
      }
    }
  }
}

void damage_any_collider_by_type(Character2D *from_actor, actor_type to_type) {
  if (!from_actor) {
    TraceLog(LOG_ERROR, "ERROR::game_manager::check_any_collide()::Tried to "
                        "check collide with uninitialized actor");
  }
  switch (to_type) {
  case ENEMY: {
    for (u16 i = 1; i <= game_manager_state->spawn_collision_count; ++i) {
      if (game_manager_state->spawn_collisions[i].is_active){
        if (CheckCollisionRecs(game_manager_state->spawn_collisions[i].rect, from_actor->collision)) {
          u16 result = damage_spawn(game_manager_state->spawn_collisions[i].owner_id, from_actor->damage);
        }
      }
    }
    break;
  }
  case PLAYER: {
    if (CheckCollisionRecs(from_actor->collision, get_player_state()->collision)) {
      event_fire(EVENT_CODE_PLAYER_DEAL_DAMAGE, 0, (event_context) { .data.u8[0] = from_actor->damage});
      return;
    }
    break;
  }
  case PROJECTILE_ENEMY: break; // TODO: Warn
  case PROJECTILE_PLAYER: break; // TODO: Warn
  }
}

Vector2 focus_at() {
  switch (get_current_scene_type()) {
  case SCENE_MAIN_MENU:
    return (Vector2) {0, 0};
  case SCENE_IN_GAME:
    return get_player_position(false);
  case SCENE_IN_GAME_EDIT:
    return get_spectator_position();
  default: break;
  }

  TraceLog(LOG_WARNING, "WARNING::game_manager::focus_at()::Scene type:%d cannot handling correctly", get_current_scene_type());
  return (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2};
}
scene_type get_active_scene() {
  return get_current_scene_type();
}
