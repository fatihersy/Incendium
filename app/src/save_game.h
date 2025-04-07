#ifndef SAVE_GAME_H
#define SAVE_GAME_H

#include "defines.h"

typedef struct save_data {
  u8 file_name[MAX_FILENAME_LENGTH];
  u32 currency_souls_player_have;
  player_state p_player;
  bool is_success;
} save_data;

typedef enum save_slots {
  SAVE_SLOT_UNDEFINED,
  SAVE_SLOT_CURRENT_SESSION,
  SAVE_SLOT_1,
  SAVE_SLOT_2,
  SAVE_SLOT_3,
  SAVE_SLOT_MAX,
}save_slots;


void save_system_initialize(void);

bool parse_or_create_save_data_from_file(save_slots slot);
bool save_save_data(save_slots slot);

//bool set_resolution(u32 width, u32 height);

save_data* get_save_data(save_slots slot);

#endif
