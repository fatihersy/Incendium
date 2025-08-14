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

typedef struct save_data {
  save_slot_id id;
  std::string file_name;
  i32 currency_souls_player_have;
  player_state player_data;
  bool is_success;
  save_data(void) {
    this->id = SAVE_SLOT_UNDEFINED;
    this->file_name = std::string("");
    this->currency_souls_player_have = 0;
    this->player_data = player_state();
    this->is_success = false;
  }
  save_data(std::string filename) : save_data() {
    this->file_name = filename;
  }
  save_data(save_slot_id id, player_state in_player_state, i32 currency_souls) : save_data() {
    this->id = id;
    this->player_data = in_player_state;
    this->currency_souls_player_have = currency_souls;
  }
} save_data;

bool save_system_initialize(void);

bool parse_or_create_save_data_from_file(save_slot_id slot, save_data default_save);
bool save_save_data(save_slot_id slot);
bool does_save_exist(save_slot_id slot);

save_data* get_save_data(save_slot_id slot);

#endif
