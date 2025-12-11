#ifndef SAVE_GAME_H
#define SAVE_GAME_H

#include "game/game_types.h"

bool save_system_initialize(void);

bool parse_or_create_save_data_from_file(save_slot_id slot, save_data default_save);
bool save_save_data(save_slot_id slot);
bool parse_save_data(save_slot_id slot, save_data default_save);
bool does_save_exist(save_slot_id slot);

save_data& get_save_data(save_slot_id slot);

#endif
