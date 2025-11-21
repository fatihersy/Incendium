#include "player.h"
#include <algorithm>

#include "core/event.h"
#include "core/fmemory.h"
#include "core/logger.h"
#include "core/fmath.h"

//#include "loc_types.h"

#include "game/spritesheet.h"

// To avoid dublicate symbol errors. Implementation in defines.h
extern const i32 level_curve[MAX_PLAYER_LEVEL+1];
#define PLAYER_DAMAGE_BREAK_TIME .5f
#define PLAYER_COMBO_TIMEOUT 0.85f
#define PLAYER_ROLL_DURATION 0.45f
#define NORMAL_ATTACK_DIM_SCALE (Vector2 {1.5f, 1.5f})

typedef struct player_system_state {
  const camera_metrics * in_camera_metrics;
  const app_settings * in_app_settings;
  const ingame_info * in_ingame_info;

  player_state dynamic_player;
  player_state defualt_player;

  f32 combo_timeout_accumulator;

  player_system_state(void) {
    this->in_camera_metrics = nullptr;
    this->in_app_settings = nullptr;
    this->in_ingame_info = nullptr;

    this->dynamic_player = player_state();
    this->defualt_player = player_state();

    this->combo_timeout_accumulator = 0.f;
  }
} player_system_state;

static player_system_state* state = nullptr;

bool player_system_on_event(i32 code, event_context context);
void player_system_reinit(void);
void play_anim(spritesheet_id player_anim_sheet);
void player_attack_reset(bool _retrospective);
void player_update_sprite(void);
void player_update_attack(void);
void player_update_movement(Vector2& move_delta);
void player_update_roll(Vector2& move_delta);

Vector2 get_player_direction(void);
void player_damage_break_init(const f32 duration);
void player_damage_break_update(void); 

bool player_system_initialize(const camera_metrics* in_camera_metrics,const app_settings* in_app_settings,const ingame_info* in_ingame_info) {
  if (state and state != nullptr){ 
    player_system_reinit();
    return true;
  }
  state = (player_system_state*)allocate_memory_linear(sizeof(player_system_state), true);
  if (not state or state == nullptr) {
    IERROR("player::player_system_initialize()::Failed to allocate player system");
    return false;
  }
  *state = player_system_state();

  if (not in_camera_metrics or in_camera_metrics == nullptr) {
    IERROR("player::player_system_initialize()::Camera metric pointer is invalid");
    return false;
  }
  if (not in_app_settings or in_app_settings == nullptr) {
    IERROR("player::player_system_initialize()::Settings pointer is invalid");
    return false;
  }
  if (not in_ingame_info or in_ingame_info == nullptr) {
    IERROR("player::player_system_initialize()::Game info pointer is invalid");
    return false;
  }

  state->in_camera_metrics = in_camera_metrics;
  state->in_app_settings = in_app_settings;
  state->in_ingame_info = in_ingame_info;

  state->defualt_player.move_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_MOVE_RIGHT;
  state->defualt_player.idle_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_IDLE_RIGHT;
  state->defualt_player.take_damage_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT;
  state->defualt_player.wreck_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_WRECK_RIGHT;
  state->defualt_player.attack_1_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_ATTACK_1;
  state->defualt_player.attack_2_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_ATTACK_2;
  state->defualt_player.attack_3_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_ATTACK_3;
  state->defualt_player.attack_down_sprite.sheet_id =  SHEET_ID_PLAYER_ANIMATION_ATTACK_DOWN;
  state->defualt_player.attack_up_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_ATTACK_UP;
  state->defualt_player.roll_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_ROLL;
  state->defualt_player.dash_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_DASH;
  set_sprite(&state->defualt_player.move_right_sprite,        true, false);
  set_sprite(&state->defualt_player.idle_right_sprite,        true, false);
  set_sprite(&state->defualt_player.take_damage_right_sprite, true, false);
  set_sprite(&state->defualt_player.wreck_right_sprite,       false, false);
  set_sprite(&state->defualt_player.attack_1_sprite,          false, false);
  set_sprite(&state->defualt_player.attack_2_sprite,          false, false);
  set_sprite(&state->defualt_player.attack_3_sprite,          false, false);
  set_sprite(&state->defualt_player.attack_down_sprite,       false, false);
  set_sprite(&state->defualt_player.attack_up_sprite,         false, false);
  set_sprite(&state->defualt_player.roll_sprite,              false, false);
  set_sprite(&state->defualt_player.dash_sprite,              false, false);

  {
    auto set_character_stat = [](character_stat_id id, ::data_type _data_type, data128 default_value){
      state->defualt_player.stats[id] = character_stat(id, 0, 0, ZERORECT, 1, 100, _data_type, default_value);
    };
    set_character_stat(CHARACTER_STATS_HEALTH,                    DATA_TYPE_I32, data128(999999));
    set_character_stat(CHARACTER_STATS_STAMINA,                   DATA_TYPE_I32, data128(100));
    set_character_stat(CHARACTER_STATS_MANA,                      DATA_TYPE_I32, data128(100));
    set_character_stat(CHARACTER_STATS_HP_REGEN,                  DATA_TYPE_I32, data128());
    set_character_stat(CHARACTER_STATS_STAMINA_REGEN,             DATA_TYPE_I32, data128(10));
    set_character_stat(CHARACTER_STATS_MANA_REGEN,                DATA_TYPE_I32, data128(10));
    set_character_stat(CHARACTER_STATS_MOVE_SPEED,                DATA_TYPE_F32, data128(128.00f));
    set_character_stat(CHARACTER_STATS_AOE,                       DATA_TYPE_F32, data128());
    set_character_stat(CHARACTER_STATS_OVERALL_DAMAGE,            DATA_TYPE_F32, data128());
    set_character_stat(CHARACTER_STATS_ABILITY_CD,                DATA_TYPE_F32, data128());
    set_character_stat(CHARACTER_STATS_PROJECTILE_AMOUNT,         DATA_TYPE_I32, data128());
    set_character_stat(CHARACTER_STATS_EXP_GAIN,                  DATA_TYPE_F32, data128());
    set_character_stat(CHARACTER_STATS_TOTAL_TRAIT_POINTS,        DATA_TYPE_I32, data128(7));
    set_character_stat(CHARACTER_STATS_BASIC_ATTACK_DAMAGE,       DATA_TYPE_I32, data128(40));
    set_character_stat(CHARACTER_STATS_BASIC_ATTACK_SPEED,        DATA_TYPE_F32, data128(.6f));
    set_character_stat(CHARACTER_STATS_CRITICAL_CHANCE,           DATA_TYPE_F32, data128());
    set_character_stat(CHARACTER_STATS_CRITICAL_DAMAGE,           DATA_TYPE_F32, data128());
    set_character_stat(CHARACTER_STATS_OVERALL_LUCK,              DATA_TYPE_F32, data128());
    set_character_stat(CHARACTER_STATS_DAMAGE_REDUCTION,          DATA_TYPE_F32, data128());
    set_character_stat(CHARACTER_STATS_CONDITION_DURATION,        DATA_TYPE_F32, data128());
    set_character_stat(CHARACTER_STATS_DAMAGE_OVER_TIME,          DATA_TYPE_I32, data128());
    set_character_stat(CHARACTER_STATS_DAMAGE_DEFERRAL,           DATA_TYPE_F32, data128());
    set_character_stat(CHARACTER_STATS_SIGIL_EFFECTIVENESS,       DATA_TYPE_F32, data128());
    set_character_stat(CHARACTER_STATS_VITAL_SIGIL_EFFECTIVENESS, DATA_TYPE_F32, data128());
    set_character_stat(CHARACTER_STATS_LETAL_SIGIL_EFFECTIVENESS, DATA_TYPE_F32, data128());
    set_character_stat(CHARACTER_STATS_DROP_RATE,                 DATA_TYPE_F32, data128());
    set_character_stat(CHARACTER_STATS_REWARD_MODIFIER,           DATA_TYPE_F32, data128());
  }

  state->defualt_player.collision.width  = (state->defualt_player.idle_right_sprite.coord.width  * .9f) * PLAYER_PLAYER_SCALE; // INFO: player collision scales with idle spritesheet
  state->defualt_player.collision.height = (state->defualt_player.idle_right_sprite.coord.height * .9f) * PLAYER_PLAYER_SCALE;
  state->defualt_player.position = ZEROVEC2;
  state->defualt_player.is_dead = false;
  state->defualt_player.w_direction = WORLD_DIRECTION_LEFT;
  state->defualt_player.is_damagable = true;
  state->defualt_player.damage_break_duration = PLAYER_DAMAGE_BREAK_TIME;
  state->defualt_player.damage_break_accumulator = 0.f;
  state->defualt_player.level = 1;
  state->defualt_player.exp_to_next_level = level_curve[state->defualt_player.level];
  state->defualt_player.exp_current = 0;
  state->defualt_player.health_current = 0;
  state->defualt_player.health_perc = 0.f;
  state->defualt_player.is_initialized = true;
  state->defualt_player.roll_sprite.fps = state->defualt_player.roll_sprite.frame_total / PLAYER_ROLL_DURATION;
  
  event_register(EVENT_CODE_PLAYER_ADD_EXP, player_system_on_event);
  event_register(EVENT_CODE_PLAYER_SET_POSITION, player_system_on_event);
  event_register(EVENT_CODE_PLAYER_TAKE_DAMAGE, player_system_on_event);

  player_system_reinit();

  return true;
}
void player_system_reinit(void) {
  if (not state or state == nullptr) {
    IERROR("player::player_system_reinit()::State is invalid");
    return;
  }
  //state->dynamic_player = state->defualt_player;
}

player_update_results update_player(void) {
  player_update_results results = player_update_results();

  if (not state or state == nullptr or state->dynamic_player.is_dead) return results;

  player_state& _player = state->dynamic_player;

  if (_player.anim_state <= PL_ANIM_STATE_UNDEFINED or _player.anim_state >= PL_ANIM_STATE_MAX) {
    _player.anim_state = PL_ANIM_STATE_IDLE;
  }

  player_damage_break_update();

  if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    player_attack_init();
  }
  if (IsKeyReleased(KEY_SPACE)) {
    player_roll_init();
  }
  player_update_movement(results.move_request);
  player_update_attack();
  player_update_sprite();

  const i32 elapsed_time = static_cast<i32>(GetTime());
  i32& last_time_hp_regen_fired = state->dynamic_player.stats.at(CHARACTER_STATS_HP_REGEN).mm_ex.i32[0];
  const i32& hp_max = state->dynamic_player.stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3];
  i32& hp_current = state->dynamic_player.health_current;

  if (last_time_hp_regen_fired < elapsed_time and hp_current < hp_max) {
    last_time_hp_regen_fired = elapsed_time;
    const i32& hp_regen_amouth = state->dynamic_player.stats.at(CHARACTER_STATS_HP_REGEN).buffer.i32[3];
    hp_current += hp_regen_amouth;
    hp_current = std::clamp(hp_current, 0, hp_max);
  }
  _player.health_perc = static_cast<f32>(_player.health_current) / static_cast<f32>(_player.stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3]);

  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_EXPERIANCE, (f32)state->dynamic_player.exp_perc));
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_HEALTH, (f32)state->dynamic_player.health_perc));
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_MANA, (f32)state->dynamic_player.mana_perc));
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_STAMINA, (f32)state->dynamic_player.stamina_perc));

  results.is_success = true;
  return results;
}
bool render_player(void) {
  if (not state or state == nullptr) { return false; }
  
  spritesheet& _sheet = state->dynamic_player.current_anim_to_play;
  const Rectangle& frame_rect = state->dynamic_player.current_anim_to_play.current_frame_rect;
  
  if(_sheet.sheet_id > SHEET_ID_SPRITESHEET_UNSPECIFIED and _sheet.sheet_id < SHEET_ID_SPRITESHEET_TYPE_MAX) {
    play_sprite_on_site_ex(__builtin_addressof(_sheet), Rectangle {frame_rect.x + _sheet.offset.x, frame_rect.y + _sheet.offset.y, frame_rect.width, frame_rect.height}, 
      state->dynamic_player.current_anim_to_play.coord, 
      state->dynamic_player.current_anim_to_play.origin, 
      state->dynamic_player.current_anim_to_play.rotation, 
      WHITE
    ); 
  }
  #if DEBUG_COLLISIONS
    DrawRectangleLines(
      static_cast<i32>(state->dynamic_player.collision.x), static_cast<i32>(state->dynamic_player.collision.y),
      static_cast<i32>(state->dynamic_player.collision.width), static_cast<i32>(state->dynamic_player.collision.height), 
      WHITE
    );
  #endif
  return true;
}

void player_move_player(Vector2 new_pos) {
  if (not state or state == nullptr) {
    IERROR("player::player_move_player()::State is invalid");
    return;
  }
  player_state& _player = state->dynamic_player;
  if (_player.is_dead or _player.anim_state <= PL_ANIM_STATE_UNDEFINED or _player.anim_state >= PL_ANIM_STATE_MAX) {
    return;
  }
  if (new_pos.x < 0.f) {
    _player.w_direction = WORLD_DIRECTION_LEFT;
  }
  else if (new_pos.x > 0.f) {
    _player.w_direction = WORLD_DIRECTION_RIGHT;
  }
  
  const bool is_moving = (new_pos.x != 0.f or new_pos.y != 0.f);
  const bool can_change_anim = (_player.anim_state != PL_ANIM_STATE_ATTACK and _player.anim_state != PL_ANIM_STATE_ROLL);

  if (can_change_anim) {
    if (is_moving) {
      _player.anim_state = PL_ANIM_STATE_WALK;
    } else {
      _player.anim_state = PL_ANIM_STATE_IDLE;
    }
  }

  _player.position.x += new_pos.x;
  _player.position.y += new_pos.y;

  const Rectangle& bounds = state->in_ingame_info->current_map_info->level_bound;
  _player.position.x = std::clamp(_player.position.x, bounds.x, bounds.x + bounds.width);
  _player.position.y = std::clamp(_player.position.y, bounds.y, bounds.y + bounds.height);

  _player.collision = Rectangle {
    _player.position.x - (_player.collision.width * .5f),
    _player.position.y - (_player.collision.height * .5f),
    _player.collision.width,
    _player.collision.height
  };
  _player.map_level_collision = Rectangle {
    _player.collision.x + (_player.collision.width * .25f),
    _player.collision.y + (_player.collision.height * .8f),
    _player.collision.width * .5f,
    _player.collision.height * .2f
  };
}
void player_add_exp_to_player(i32 raw_exp) {
  if (not state or state == nullptr) { return; }

  i32 curr = state->dynamic_player.exp_current;
  i32 to_next = state->dynamic_player.exp_to_next_level;
  i32 exp = raw_exp + (raw_exp * state->dynamic_player.stats.at(CHARACTER_STATS_EXP_GAIN).buffer.f32[3]);

  if ( curr + exp >= to_next) 
  {
    state->dynamic_player.exp_current = (curr + exp) - to_next;
    state->dynamic_player.level++;
    state->dynamic_player.exp_to_next_level = level_curve[state->dynamic_player.level];
    state->dynamic_player.is_player_have_ability_upgrade_points = true;
  }
  else {
    state->dynamic_player.exp_current += exp;
  }
  state->dynamic_player.exp_perc = static_cast<f32>(state->dynamic_player.exp_current) / static_cast<f32>(state->dynamic_player.exp_to_next_level);
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_EXPERIANCE, (f32)state->dynamic_player.exp_perc));
}
void player_take_damage(i32 damage) {
  if (not state or state == nullptr) { return; }

  if(not state->dynamic_player.is_damagable or state->dynamic_player.is_dead) return;
  if((state->dynamic_player.health_current - damage) > 0 and (state->dynamic_player.health_current - damage) <= state->dynamic_player.stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3]) {
    //i32 health_prev = state->dynamic_player.health_current;
    state->dynamic_player.health_current -= damage;
    player_damage_break_init(PLAYER_DAMAGE_BREAK_TIME);
    //TraceLog(LOG_INFO, "%.1f -- Player Health : %d - %d = %d", GetTime(), health_prev, damage, state->dynamic_player.health_current);
  }
  else {
    state->dynamic_player.health_current = 0;
    state->dynamic_player.is_dead = true;
    event_fire(EVENT_CODE_END_GAME, event_context(static_cast<i32>(false)));
  }
}
void player_heal_player(i32 amouth){
  if (not state or state == nullptr) { return; }

  if(state->dynamic_player.is_dead) return;
  if(state->dynamic_player.health_current + amouth <= state->dynamic_player.stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3]) {
    state->dynamic_player.health_current += amouth;
  }
  else {
    state->dynamic_player.health_current = state->dynamic_player.stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3];
  }
  state->dynamic_player.health_perc = static_cast<f32>(state->dynamic_player.health_current) / static_cast<f32>(state->dynamic_player.stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3]);
}
void player_update_sprite(void) {
  if (not state or state == nullptr) { return; }
  player_state& _player = state->dynamic_player;

  auto set_sheet_if_not_already = [&_player](spritesheet& sheet_to_set) {
    if (_player.current_anim_to_play.sheet_id != sheet_to_set.sheet_id) {
      _player.current_anim_to_play = sheet_to_set;
    }
  };
  switch (_player.anim_state) {
    case PL_ANIM_STATE_IDLE: set_sheet_if_not_already(_player.idle_right_sprite); break;
    case PL_ANIM_STATE_WALK: set_sheet_if_not_already(_player.move_right_sprite); break;
    case PL_ANIM_STATE_ROLL: set_sheet_if_not_already(_player.roll_sprite); break;
    case PL_ANIM_STATE_ATTACK: {
      switch (_player.combo_type) {
        case CHARACTER_ATTACK_COMBO_1: set_sheet_if_not_already(_player.attack_1_sprite); break;
        case CHARACTER_ATTACK_COMBO_2: set_sheet_if_not_already(_player.attack_2_sprite); break;
        case CHARACTER_ATTACK_COMBO_3: set_sheet_if_not_already(_player.attack_3_sprite); break;
        default: return;
      }
      break;
    }
    case PL_ANIM_STATE_TAKE_DAMAGE: set_sheet_if_not_already(_player.take_damage_right_sprite); break;
    default: {
      return;
    }
  }
  spritesheet& _sheet = _player.current_anim_to_play;
  if (_player.w_direction == WORLD_DIRECTION_LEFT && _sheet.current_frame_rect.width > 0) {
    _sheet.current_frame_rect.width *= -1;
  }
  if (_player.w_direction == WORLD_DIRECTION_RIGHT && _sheet.current_frame_rect.width < 0) {
    _sheet.current_frame_rect.width *= -1;
  }
  _sheet.coord = Rectangle { 
    _player.position.x, _player.position.y,
    std::abs(_sheet.current_frame_rect.width) * PLAYER_PLAYER_SCALE, std::abs(_sheet.current_frame_rect.height) * PLAYER_PLAYER_SCALE
  };
  _sheet.origin = Vector2 { _sheet.coord.width * .5f, _sheet.coord.height * .5f};
  
  update_sprite(__builtin_addressof(_sheet), (*state->in_ingame_info->delta_time));
}
void player_update_attack(void) {
  if (not state or state == nullptr) { return; }

  player_state& _player = state->dynamic_player;

  if (state->combo_timeout_accumulator > 0.f) {
    state->combo_timeout_accumulator -= (*state->in_ingame_info->delta_time);
    if (state->combo_timeout_accumulator <= 0.f) {
      player_attack_reset(true);
    }
  }
  if (_player.anim_state != PL_ANIM_STATE_ATTACK) {
    return;
  }
  if (_player.combo_type <= CHARACTER_ATTACK_COMBO_UNDEFINED or _player.combo_type >= CHARACTER_ATTACK_COMBO_MAX) {
    player_attack_reset(true);
  }

  if (_player.current_anim_to_play.is_played) {
    player_attack_reset(false);
    state->combo_timeout_accumulator = PLAYER_COMBO_TIMEOUT;
    Rectangle damage_area = Rectangle {
      _player.position.x,
      _player.position.y,
      _player.collision.width * NORMAL_ATTACK_DIM_SCALE.x,
      _player.collision.height * NORMAL_ATTACK_DIM_SCALE.y
    };

    f32 direction = (_player.w_direction == WORLD_DIRECTION_RIGHT) ? 1.0f : -1.0f;
    damage_area.x -= damage_area.width * .5f;
    damage_area.y -= damage_area.height * .5f;

    damage_area.x += direction * damage_area.width;

    event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, event_context(
      static_cast<i16>(damage_area.x),
      static_cast<i16>(damage_area.y),
      static_cast<i16>(damage_area.width),
      static_cast<i16>(damage_area.height),
      static_cast<i16>(_player.stats.at(CHARACTER_STATS_OVERALL_DAMAGE).buffer.i32[3]),
      static_cast<i16>(COLLISION_TYPE_RECTANGLE_RECTANGLE)
    ));
  }
}
void player_attack_init(void) {
  if (not state or state == nullptr) { return; }

  player_state& _player = state->dynamic_player;
  if (_player.is_dead) {
    return;
  }
  if (_player.anim_state < PL_ANIM_STATE_UNDEFINED and _player.anim_state > PL_ANIM_STATE_MAX) {
    IWARN("player::player_attack_init()::Player state is out of bound");
    return;
  }
  const bool able_to_attack = _player.anim_state == PL_ANIM_STATE_IDLE or _player.anim_state == PL_ANIM_STATE_WALK;
  if (not able_to_attack) {
    return;
  }
  _player.anim_state = PL_ANIM_STATE_ATTACK;

  switch (_player.combo_type) {
    case CHARACTER_ATTACK_COMBO_UNDEFINED: {
      _player.combo_type = CHARACTER_ATTACK_COMBO_1;
      return;
    }
    case CHARACTER_ATTACK_COMBO_1: {
      _player.combo_type = CHARACTER_ATTACK_COMBO_2;
      return;
    }
    case CHARACTER_ATTACK_COMBO_2: {
      _player.combo_type = CHARACTER_ATTACK_COMBO_3;
      return;
    }
    case CHARACTER_ATTACK_COMBO_3: { player_attack_reset(true); return; }
    default: {
      IWARN("player::player_attack_init()::Unsupported combo type");
      player_attack_reset(true);
      return;
    }
  }
  IERROR("player::player_attack_init()::Function ended unexpectedly");
}
void player_attack_reset(bool _retrospective) {
  if (not state or state == nullptr) { return; }

  if (_retrospective) {
    state->dynamic_player.combo_type = CHARACTER_ATTACK_COMBO_UNDEFINED;
    state->combo_timeout_accumulator = 0.f;
  }
  state->dynamic_player.anim_state = PL_ANIM_STATE_IDLE;

  switch (state->dynamic_player.combo_type) {
    case CHARACTER_ATTACK_COMBO_1: { reset_sprite(&state->dynamic_player.attack_1_sprite, true); return; }
    case CHARACTER_ATTACK_COMBO_2: { reset_sprite(&state->dynamic_player.attack_2_sprite, true); return; }
    case CHARACTER_ATTACK_COMBO_3: { reset_sprite(&state->dynamic_player.attack_3_sprite, true); return; }
    default: {
      reset_sprite(&state->dynamic_player.attack_1_sprite, true);
      reset_sprite(&state->dynamic_player.attack_2_sprite, true);
      reset_sprite(&state->dynamic_player.attack_3_sprite, true);
      state->dynamic_player.combo_type = CHARACTER_ATTACK_COMBO_UNDEFINED;
      state->combo_timeout_accumulator = 0.f;
      return;
    }
  }
}
void player_update_movement(Vector2& move_delta) {
  if (not state or state == nullptr) { return; }

  player_state& _player = state->dynamic_player;
  if (_player.is_dead) {
    return;
  }

  if (_player.anim_state == PL_ANIM_STATE_ROLL) {
    player_update_roll(move_delta);
    return;
  }

  const bool able_to_walk = _player.anim_state == PL_ANIM_STATE_IDLE or _player.anim_state == PL_ANIM_STATE_WALK;
  if (not able_to_walk) {
    return;
  }
  move_delta = vec2_scale(get_player_direction(), _player.stats.at(CHARACTER_STATS_MOVE_SPEED).buffer.f32[3] * (*state->in_ingame_info->delta_time));
}
Vector2 get_player_direction(void) {
  Vector2 out_delta = ZEROVEC2;

  if (IsKeyDown(KEY_W)) {
    out_delta.y += DIRECTION_VECTOR_UP.y;
  }
  if (IsKeyDown(KEY_A)) {
    out_delta.x += DIRECTION_VECTOR_LEFT.x;
  }
  if (IsKeyDown(KEY_S)) {
    out_delta.y += DIRECTION_VECTOR_DOWN.y;
  }
  if (IsKeyDown(KEY_D)) {
    out_delta.x += DIRECTION_VECTOR_RIGHT.x;
  }
  return vec2_normalize(out_delta);
}
void player_roll_init(void) {
  if (not state or state == nullptr) { return; }

  player_state& _player = state->dynamic_player;
  if (_player.is_dead) {
    return;
  }
  if (_player.anim_state < PL_ANIM_STATE_UNDEFINED and _player.anim_state > PL_ANIM_STATE_MAX) {
    IWARN("player::player_attack_init()::Player state is out of bound");
    return;
  }
  const bool able_to_roll = _player.anim_state == PL_ANIM_STATE_IDLE or _player.anim_state == PL_ANIM_STATE_WALK or _player.anim_state == PL_ANIM_STATE_ATTACK;
  if (not able_to_roll) {
    return;
  }
  if (_player.anim_state == PL_ANIM_STATE_ATTACK) {
    player_attack_reset(false);
  }
  _player.anim_state = PL_ANIM_STATE_ROLL;

  _player.roll_control = easing_accumulation_control();
  _player.roll_control.duration = PLAYER_ROLL_DURATION;
  _player.roll_control.begin_x = _player.position.x;
  _player.roll_control.begin_y = _player.position.y;
  _player.roll_control.ease_type = EASING_TYPE_SINE_INOUT;
  player_damage_break_init(PLAYER_ROLL_DURATION);

  Vector2 player_direction = get_player_direction();
  if (player_direction.x == 0.f and player_direction.y == 0.f) {
    switch (_player.w_direction) {
      case WORLD_DIRECTION_LEFT:  player_direction = DIRECTION_VECTOR_LEFT; break;
      case WORLD_DIRECTION_RIGHT: player_direction = DIRECTION_VECTOR_RIGHT; break;
      default: return;
    }
  }
  const f32 roll_distance = _player.stats.at(CHARACTER_STATS_MOVE_SPEED).buffer.f32[3] * PLAYER_ROLL_DURATION * 1.5f;
  _player.roll_control.change_x = player_direction.x * roll_distance;
  _player.roll_control.change_y = player_direction.y * roll_distance;
}
void player_update_roll(Vector2& move_delta) {
  if (not state or state == nullptr) { return; }

  player_state& _player = state->dynamic_player;

  if (_player.anim_state != PL_ANIM_STATE_ROLL) {
    return;
  }
  _player.roll_control.accumulator += (*state->in_ingame_info->delta_time);
  if (_player.roll_control.accumulator >= _player.roll_control.duration) {
    _player.roll_control = easing_accumulation_control();
    _player.anim_state = PL_ANIM_STATE_IDLE;
    return;
  }
  easing_accumulation_control& ease = _player.roll_control;

  move_delta.x = math_easing(ease.accumulator, ease.begin_x, ease.change_x, ease.duration, ease.ease_type) - _player.position.x;
  move_delta.y = math_easing(ease.accumulator, ease.begin_y, ease.change_y, ease.duration, ease.ease_type) - _player.position.y;
}
void player_damage_break_init(const f32 duration) {
  state->dynamic_player.is_damagable = false;
  state->dynamic_player.damage_break_duration = duration;
}
void player_damage_break_update(void) {
  if (not state or state == nullptr) {
    return;
  }
  player_state& _player = state->dynamic_player;

  if(_player.is_damagable) {
    return;
  }
  if(_player.damage_break_accumulator >= _player.damage_break_duration) { 
    _player.is_damagable = true;
    _player.damage_break_accumulator = 0.f;
    return;
  }
  _player.damage_break_accumulator += (*state->in_ingame_info->delta_time);
}

player_state* get_player_state(void) {
  if (not state or state == nullptr) {
    return nullptr;
  }
  return __builtin_addressof(state->dynamic_player);
}
void set_player_state(player_state _player_state) {
  if (not state or state == nullptr) {
    IERROR("player::set_player_state()::State is invalid");
    return;
  }
  state->dynamic_player = _player_state;
}
const player_state* get_default_player(void) {
  if (not state or state == nullptr) {
    return nullptr;
  }
  return __builtin_addressof(state->defualt_player);
}

bool player_system_on_event(i32 code, event_context context) {
    switch (code) {
        case EVENT_CODE_PLAYER_ADD_EXP: {
          player_add_exp_to_player(context.data.i32[0]);
          return true;
        }
        case EVENT_CODE_PLAYER_SET_POSITION: {
          state->dynamic_player.position.x = context.data.f32[0]; 
          state->dynamic_player.position.y = context.data.f32[1]; 
          state->dynamic_player.collision.x = state->dynamic_player.position.x; 
          state->dynamic_player.collision.y = state->dynamic_player.position.y; 
          return true;
        }
        case EVENT_CODE_PLAYER_TAKE_DAMAGE: {
          player_take_damage(context.data.i32[0]);
          return true;
        }
        case EVENT_CODE_PLAYER_HEAL: {
          player_heal_player(context.data.i32[0]);
          return true;
        }
        default: return false; // TODO: Warn
    }

    return false;
}
