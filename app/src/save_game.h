#ifndef SAVE_GAME_H
#define SAVE_GAME_H

#include "game/game_types.h"

typedef enum save_slot_id {
  SAVE_SLOT_UNDEFINED,
  SAVE_SLOT_CURRENT_SESSION,
  SAVE_SLOT_1,
  SAVE_SLOT_2,
  SAVE_SLOT_3,
  SAVE_SLOT_MAX,
}save_slot_id;

// Save data structure
//struct save_data {
//  save_slot_id id = SAVE_SLOT_UNDEFINED;
//  std::string file_name;
//  bool is_success = false;
//  int32_t currency_coins_player_have = 0;
//  struct player_stats {
//    std::array<int32_t, CHARACTER_STATS_COUNT> stats = {0}; // HEALTH, HP_REGEN, etc.
//  } player_data;
//};

typedef struct save_data {
  save_slot_id id;
  std::string file_name;
  i32 currency_coins_player_have;
  i32 currency_souls_player_have;
  player_state player_data;
  std::array<game_rule, GAME_RULE_MAX> game_rules;
  std::array<sigil_slot, SIGIL_SLOT_MAX> sigil_slots;

  bool is_success;
  save_data(void) {
    this->id = SAVE_SLOT_UNDEFINED;
    this->file_name = std::string();
    this->currency_coins_player_have = 0;
    this->player_data = player_state();
    this->game_rules.fill(game_rule());
    this->is_success = false;
  }
  save_data(std::string filename) : save_data() {
    this->file_name = filename;
  }
  save_data(save_slot_id id, player_state in_player_state, std::array<game_rule, GAME_RULE_MAX>& _game_rules, i32 currency_coins) : save_data() {
    this->id = id;
    this->player_data = in_player_state;
    this->game_rules = _game_rules;
    this->currency_coins_player_have = currency_coins;
  }
} save_data;

bool save_system_initialize(void);

bool parse_or_create_save_data_from_file(save_slot_id slot, save_data default_save);
bool save_save_data(save_slot_id slot);
bool does_save_exist(save_slot_id slot);

save_data* get_save_data(save_slot_id slot);

#endif
