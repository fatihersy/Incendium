
#include "collectible_manager.h"
#include <reasings.h>
#include <sound.h>

#include "core/logger.h"
#include "core/event.h"

#include "game/spritesheet.h"

typedef struct collectible_manager_system_state {
  const camera_metrics* in_camera_metrics;
  const app_settings* in_app_settings;
  const tilemap ** in_active_map;
  const ingame_info * in_ingame_info;

  std::vector<loot_item> loots_on_the_map;
  i32 next_item_id {};

  collectible_manager_system_state(void) {
		this->in_camera_metrics = nullptr;
		this->in_app_settings = nullptr;
		this->in_active_map = nullptr;
    this->in_ingame_info = nullptr;
  }
} collectible_manager_system_state;

static collectible_manager_system_state * state = nullptr;

#define LOOT_ITEM_SCALE 2.f
#define LOOT_DROP_ANIMATION_DURATION .85f
#define LOOT_GRAB_ANIMATION_CURVE_CONTROL_OFFSET 50.f

bool collectible_manager_reinit(const camera_metrics * in_camera_metrics, const app_settings * in_app_settings, const tilemap ** const in_active_map_ptr, const ingame_info * in_ingame_info);
bool collectible_manager_on_event(i32 code, [[__maybe_unused__]] event_context context);

bool loot_item_on_loot(item_type type, i32 id, data128 context);

[[__nodiscard__]] bool collectible_manager_initialize(
	const camera_metrics* _camera_metrics, 
	const app_settings * in_app_settings, 
	const tilemap ** const in_active_map_ptr, 
	const ingame_info * in_ingame_info
) {
	if (state and state != nullptr) {
    return collectible_manager_reinit(_camera_metrics, in_app_settings, in_active_map_ptr, in_ingame_info);
  }
  state = (collectible_manager_system_state *)allocate_memory_linear(sizeof(collectible_manager_system_state), true);
  if (not state or state == nullptr) {
    IERROR("collectible_manager::collectible_manager_initialize()::State allocation failed");
    return false;
  }
  *state = collectible_manager_system_state();

  event_register(EVENT_CODE_SPAWN_ITEM, collectible_manager_on_event);

  return collectible_manager_reinit(_camera_metrics, in_app_settings, in_active_map_ptr, in_ingame_info);
}

bool collectible_manager_reinit(const camera_metrics * in_camera_metrics, const app_settings * in_app_settings, const tilemap ** const in_active_map_ptr, const ingame_info * in_ingame_info) {
  if (not in_camera_metrics or in_camera_metrics == nullptr) {
    IERROR("collectible_manager::collectible_manager_initialize()::Camera pointer is invalid");
    return false;
  }
  if (not in_app_settings or in_app_settings == nullptr) {
    IERROR("collectible_manager::collectible_manager_initialize()::Settings pointer is invalid");
    return false;
  }
  if (not in_active_map_ptr or in_active_map_ptr == nullptr) {
    IERROR("collectible_manager::collectible_manager_initialize()::Map pointer is invalid");
    return false;
  }
  if (not in_ingame_info or in_ingame_info == nullptr) {
    IERROR("collectible_manager::collectible_manager_initialize()::Ingame info pointer is invalid");
    return false;
  }
	state->in_camera_metrics = in_camera_metrics;
	state->in_app_settings = in_app_settings;
	state->in_active_map = in_active_map_ptr;
  state->in_ingame_info = in_ingame_info;
	
  return true;
}

bool update_collectible_manager(void) {
	if (not state or state == nullptr) {
    IERROR("collectible_manager::update_collectible_manager()::State is not valid");
		return false;
	}
  const player_state * _player = state->in_ingame_info->player_state_dynamic;

	for (size_t itr_000 = 0; itr_000 < state->loots_on_the_map.size(); itr_000++) {
    loot_item * const item = __builtin_addressof(state->loots_on_the_map.at(itr_000));

    if (not item->is_initialized or not item->is_active) {
      continue;
    }
    loot_drop_animation_control_system& drop_control = item->drop_control;
    if (drop_control.play_animation and drop_control.drop_anim_type == LOOT_DROP_ANIMATION_PLAYER_GRAB and drop_control.accumulator > drop_control.animation_duration) {
      item->pfn_loot_item_on_loot(item->type, item->id, data128(item->mm_ex.i16[0]));
      continue;
    }

		if (CheckCollisionRecs(item->world_collision, state->in_camera_metrics->frustum)) {
      update_sprite(item->sheet, (*state->in_ingame_info->delta_time));
			item->is_on_screen = true;
      if (item->drop_control.play_animation) {

        if (drop_control.accumulator < drop_control.animation_duration) {
          switch (item->drop_control.drop_anim_type) {
            case LOOT_DROP_ANIMATION_ELASTIC_OUT: {
              item->world_collision.y = EaseElasticOut(drop_control.accumulator, drop_control.buffer.f32[0], drop_control.buffer.f32[1], drop_control.animation_duration);
              item->sheet.coord.x = item->world_collision.x;
              item->sheet.coord.y = item->world_collision.y;
              drop_control.accumulator += (*state->in_ingame_info->delta_time);
              break;
            }
            case LOOT_DROP_ANIMATION_PLAYER_GRAB: {
              Vector2 p1 = pVECTOR2(drop_control.buffer.f32);
              Vector2 p3  = Vector2 { _player->collision.x   + _player->collision.width   * .5f, _player->collision.y   + _player->collision.height   * .5f };
              Vector2 dir = Vector2 { p3.x - p1.x, p3.y - p1.y };
              f32 length = sqrtf(dir.x * dir.x + dir.y * dir.y);
              if (length != 0.0f) {
                dir.x /= length;
                dir.y /= length;
              }
              f32 offset = LOOT_GRAB_ANIMATION_CURVE_CONTROL_OFFSET;
              Vector2 c1 = Vector2{ p1.x - dir.x * offset, p1.y - dir.y * offset };
              
              Vector2 position = GetSplinePointBezierQuad(p1, c1, p3, drop_control.accumulator / drop_control.animation_duration);
              item->world_collision.x = position.x;
              item->world_collision.y = position.y;
              item->sheet.coord.x = position.x;
              item->sheet.coord.y = position.y;

              drop_control.accumulator += (*state->in_ingame_info->delta_time);
              break;
            }
            default: {
              IWARN("collectible_manager::update_collectible_manager()::Unsupported item drop animation");
              drop_control = loot_drop_animation_control_system();
              break;
            }
          }
        }
        else {
          drop_control = loot_drop_animation_control_system();
        }
      }

      switch (item->type) {
        case ITEM_TYPE_EXPERIENCE: {
          if (item->is_player_grabbed == false and CheckCollisionCircleRec(_player->position, _player->interaction_radius, item->world_collision)) {
            item->is_player_grabbed = true;
            drop_control = loot_drop_animation_control_system(LOOT_DROP_ANIMATION_PLAYER_GRAB, LOOT_DROP_ANIMATION_DURATION, item->world_collision.x, item->world_collision.y);
          }
        }
        case ITEM_TYPE_COIN: {
          if (item->is_player_grabbed == false and CheckCollisionCircleRec(_player->position, _player->interaction_radius, item->world_collision)) {
            item->is_player_grabbed = true;
            drop_control = loot_drop_animation_control_system(LOOT_DROP_ANIMATION_PLAYER_GRAB, LOOT_DROP_ANIMATION_DURATION, item->world_collision.x, item->world_collision.y);
          }
        }
        case ITEM_TYPE_HEALTH_FRAGMENT: {
          if (item->is_player_grabbed == false and CheckCollisionCircleRec(_player->position, _player->interaction_radius, item->world_collision)) {
            item->is_player_grabbed = true;
            drop_control = loot_drop_animation_control_system(LOOT_DROP_ANIMATION_PLAYER_GRAB, LOOT_DROP_ANIMATION_DURATION, item->world_collision.x, item->world_collision.y);
          }
        }
        case ITEM_TYPE_CHEST: {
          if (CheckCollisionCircleRec(_player->position, _player->interaction_radius, item->world_collision)) {
            item->pfn_loot_item_on_loot(item->type, item->id, data128(item->mm_ex.i16[0]));
          }
        }
        default: { break; }
      }

		}
	}

	return true;
}
bool render_collectible_manager(void) {
	if (not state or state == nullptr) {
    IWARN("collectible_manager::render_collectible_manager()::State is not valid");
		return false;
	}
	for (loot_item& item : state->loots_on_the_map) {
		if (item.is_on_screen) {
      switch (item.type) {
        case ITEM_TYPE_EXPERIENCE: {
          play_sprite_on_site(item.sheet, WHITE, item.world_collision);
          item.is_on_screen = false;
          continue;
        }
        case ITEM_TYPE_COIN: {
          play_sprite_on_site(item.sheet, WHITE, item.world_collision);
          item.is_on_screen = false;
          continue;
        }
        case ITEM_TYPE_HEALTH_FRAGMENT: {
          play_sprite_on_site(item.sheet, WHITE, item.world_collision);
          item.is_on_screen = false;
          continue;
        }
        case ITEM_TYPE_CHEST: {
          draw_sprite_on_site(item.sheet, WHITE, 0);
          item.is_on_screen = false;
          continue;
        }
        default: {
          return false;
        }
      }
		}
	}
	return true;
}

const loot_item * get_loot_by_id(i32 id) {
	if (id >= ITEM_TYPE_MAX or id <= ITEM_TYPE_UNDEFINED) {
    IWARN("collectible_manager::get_loot_by_id()::Id is out of bound");
		return nullptr;
	}
	for (const loot_item& item  : state->loots_on_the_map) {
		if (item.id == id) {
			return __builtin_addressof(item);
		}
	}

  IWARN("collectible_manager::get_loot_by_id()::Item cannot found");
	return nullptr;
}
const std::vector<loot_item> * get_loots_pointer(void) {
	if (not state or state == nullptr) {
    IERROR("collectible_manager::get_loots_pointer()::State is not valid");
		return nullptr;
	}

	return __builtin_addressof(state->loots_on_the_map);
}
bool remove_item(i32 id) {
	if (not state or state == nullptr) {
    IERROR("collectible_manager::remove_item()::State is not valid");
		return false;
	}
	if (id >= ITEM_TYPE_MAX or id <= ITEM_TYPE_UNDEFINED) {
    IWARN("collectible_manager::remove_item()::Id is out of bound");
		return false;
	}

	for (loot_item& item  : state->loots_on_the_map) {
		if (item.id == id) {
			item.is_active = false;
			return true;
		}
	}

  IWARN("collectible_manager::remove_item()::Item cannot found");
	return false;
}
void collectible_manager_state_clear(void) {
	if (not state or state == nullptr) {
    IERROR("collectible_manager::collectible_manager_state_clear()::State is not valid");
		return;
	}
  state->loots_on_the_map.clear();
  state->next_item_id = 0;
}
loot_item * create_loot_item(item_type type, Vector2 position, data128 context) {
  if (not state or state == nullptr) {
    IERROR("collectible_manager::create_loot_item()::State is invalid");
    return nullptr;
  }
  if (type >= ITEM_TYPE_MAX or type <= ITEM_TYPE_UNDEFINED) {
    IWARN("collectible_manager::create_loot_item()::id is out of bound");
    return nullptr;
  }
  spritesheet sheet = spritesheet();

  switch (type) {
    case ITEM_TYPE_EXPERIENCE: {
      sheet.sheet_id = SHEET_ID_LOOT_ITEM_EXPERIENCE;
      set_sprite(sheet, true, false);

      loot_item item = loot_item(type, state->next_item_id++, sheet, 
        loot_drop_animation_control_system(LOOT_DROP_ANIMATION_ELASTIC_OUT, LOOT_DROP_ANIMATION_DURATION, static_cast<f32>(context.i16[0]), static_cast<f32>(context.i16[1])), 
        true
      );
			item.world_collision.width = sheet.coord.width * LOOT_ITEM_SCALE;
			item.world_collision.height = sheet.coord.height * LOOT_ITEM_SCALE;
			item.sheet.coord.width  = item.world_collision.width;
			item.sheet.coord.height = item.world_collision.height;
			item.world_collision.x = position.x - (item.world_collision.width  * .5f);
			item.world_collision.y = position.y - (item.world_collision.height * .5f);
			item.sheet.coord.x  = item.world_collision.x;
			item.sheet.coord.y = item.world_collision.y;

			item.pfn_loot_item_on_loot = loot_item_on_loot;
      item.mm_ex.i16[0] = context.i16[2];
      item.drop_control.buffer.f32[1] -= item.world_collision.height;

      return __builtin_addressof(state->loots_on_the_map.emplace_back(item));
    }
    case ITEM_TYPE_COIN: {
      sheet.sheet_id = SHEET_ID_LOOT_ITEM_COIN;
      set_sprite(sheet, true, false);

      loot_item item = loot_item(type, state->next_item_id++, sheet, 
        loot_drop_animation_control_system(LOOT_DROP_ANIMATION_ELASTIC_OUT, LOOT_DROP_ANIMATION_DURATION, static_cast<f32>(context.i16[0]), static_cast<f32>(context.i16[1])), 
        true
      );
			item.world_collision.width  = sheet.coord.width * LOOT_ITEM_SCALE;
			item.world_collision.height = sheet.coord.height * LOOT_ITEM_SCALE;
			item.sheet.coord.width  = item.world_collision.width;
			item.sheet.coord.height = item.world_collision.height;
			item.world_collision.x = position.x - (item.world_collision.width  * .5f);
			item.world_collision.y = position.y - (item.world_collision.height * .5f);
			item.sheet.coord.x  = item.world_collision.x;
			item.sheet.coord.y = item.world_collision.y;

			item.pfn_loot_item_on_loot = loot_item_on_loot;
      item.mm_ex.i16[0] = context.i16[2];
      item.drop_control.buffer.f32[1] -= item.world_collision.height;

      return __builtin_addressof(state->loots_on_the_map.emplace_back(item));
    }
    case ITEM_TYPE_HEALTH_FRAGMENT: {
      sheet.sheet_id = SHEET_ID_LOOT_ITEM_HEALTH;
      set_sprite(sheet, true, false);

      loot_item item = loot_item(type, state->next_item_id++, sheet, 
        loot_drop_animation_control_system(LOOT_DROP_ANIMATION_ELASTIC_OUT, LOOT_DROP_ANIMATION_DURATION, static_cast<f32>(context.i16[0]), static_cast<f32>(context.i16[1])), 
        true
      );
			item.world_collision.width  = sheet.coord.width * LOOT_ITEM_SCALE;
			item.world_collision.height = sheet.coord.height * LOOT_ITEM_SCALE;
			item.sheet.coord.width  = item.world_collision.width;
			item.sheet.coord.height = item.world_collision.height;
			item.world_collision.x = position.x - (item.world_collision.width  * .5f);
			item.world_collision.y = position.y - (item.world_collision.height * .5f);
			item.sheet.coord.x  = item.world_collision.x;
			item.sheet.coord.y = item.world_collision.y;

			item.pfn_loot_item_on_loot = loot_item_on_loot;
      item.mm_ex.i16[0] = context.i16[2];
      item.drop_control.buffer.f32[1] -= item.world_collision.height;

      return __builtin_addressof(state->loots_on_the_map.emplace_back(item));
    }
    case ITEM_TYPE_CHEST: {
      sheet.sheet_id = SHEET_ID_LOOT_ITEM_CHEST;
      set_sprite(sheet, true, false);

      loot_item item = loot_item(type, state->next_item_id++, sheet, 
        loot_drop_animation_control_system(LOOT_DROP_ANIMATION_ELASTIC_OUT, LOOT_DROP_ANIMATION_DURATION, static_cast<f32>(context.i16[0]), static_cast<f32>(context.i16[1])), 
        true
      );
			item.world_collision.width  = sheet.coord.width * LOOT_ITEM_SCALE;
			item.world_collision.height = sheet.coord.height * LOOT_ITEM_SCALE;
			item.sheet.coord.width  = item.world_collision.width;
			item.sheet.coord.height = item.world_collision.height;
			item.world_collision.x = position.x - (item.world_collision.width  * .5f);
			item.world_collision.y = position.y - (item.world_collision.height * .5f);
			item.sheet.coord.x  = item.world_collision.x;
			item.sheet.coord.y = item.world_collision.y;

			item.pfn_loot_item_on_loot = loot_item_on_loot;
      item.mm_ex.i16[0] = context.i16[2];
      item.drop_control.buffer.f32[1] -= item.world_collision.height;

      return __builtin_addressof(state->loots_on_the_map.emplace_back(item));
    }
    default : {
      IWARN("collectible_manager::create_loot_item()::Unsupported id");
      return nullptr;
    }
  }

  IERROR("collectible_manager::create_loot_item()::function ended unexpectedly");
  return nullptr;
}

bool loot_item_on_loot(item_type type, i32 id, data128 context) {
  if (not state or state == nullptr) {
    IERROR("collectible_manager::loot_item_on_loot()::State is invalid");
    return false;
  }
	const loot_item * _item = nullptr;
  i32 item_index = 0;

	for (loot_item& item_ref  : state->loots_on_the_map) {
		if (item_ref.id == id and item_ref.is_active) {
			_item = __builtin_addressof(item_ref);
			break;
		}
    item_index++;
	}
	if (_item == nullptr) {
		return false;
	}

	switch (type) {
    case ITEM_TYPE_EXPERIENCE: {
      event_fire(EVENT_CODE_PLAY_SOUND, event_context(static_cast<i32>(SOUND_ID_EXP_PICKUP), static_cast<i32>(true)));

      state->loots_on_the_map.erase(state->loots_on_the_map.begin() + item_index);
			return event_fire(EVENT_CODE_PLAYER_ADD_EXP, event_context(static_cast<i32>(context.i16[0])));
		}
    case ITEM_TYPE_COIN: {
      event_fire(EVENT_CODE_PLAY_SOUND, event_context(static_cast<i32>(SOUND_ID_COIN_PICKUP)));

      state->loots_on_the_map.erase(state->loots_on_the_map.begin() + item_index);
			return event_fire(EVENT_CODE_ADD_CURRENCY_COINS, event_context(static_cast<i32>(context.i16[0])));
		}
    case ITEM_TYPE_HEALTH_FRAGMENT: {
      event_fire(EVENT_CODE_PLAY_SOUND, event_context(static_cast<i32>(SOUND_ID_HEALTH_PICKUP)));

      state->loots_on_the_map.erase(state->loots_on_the_map.begin() + item_index);
			return event_fire(EVENT_CODE_PLAYER_HEAL, event_context(static_cast<i32>(context.i16[0])));
		}
    case ITEM_TYPE_CHEST: {
      std::vector<loot_item>::const_iterator item = state->loots_on_the_map.begin() + item_index;
      Vector2 chest_screen_location = GetWorldToScreen2D(Vector2 {item->world_collision.x, item->world_collision.y}, state->in_camera_metrics->handle);
      event_fire(EVENT_CODE_BEGIN_CHEST_OPENING_SEQUENCE, event_context(chest_screen_location.x, chest_screen_location.y, static_cast<f32>(LOOT_ITEM_SCALE)));
      state->loots_on_the_map.erase(item);
			return true;
		}
		default: {
  		IWARN("collectible_manager::loot_item_on_loot()::Unsupported item type");
			return false;
		}
  }

  IERROR("collectible_manager::loot_item_on_loot()::function ended unexpectedly");
	return false;
}

bool collectible_manager_on_event(i32 code, [[__maybe_unused__]] event_context context) {
  switch (code) {
    case EVENT_CODE_SPAWN_ITEM: {
      const loot_item * item = create_loot_item(
				static_cast<item_type>(context.data.i16[0]),
				Vector2 { 
					static_cast<f32>(context.data.i16[1]), 
					static_cast<f32>(context.data.i16[2])
				},
        data128(context.data.i16[3], context.data.i16[4], context.data.i16[5], context.data.i16[6], context.data.i16[7])
			);
      
      return item->is_initialized;
    }
    default: {
      IWARN("collectible_manager::collectible_manager_on_event()::Unsuppported code.");
      return false;
    }
  }

  IERROR("collectible_manager::collectible_manager_on_event()::Function event ended unexpectedly");
  return false;
}

