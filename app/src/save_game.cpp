#include "save_game.h"
#include "core/fmemory.h"

#define TOTAL_SAVE_FILE_SIZE 2 * 1024 * 1024
#define HEADER_SYMBOL_ENTRY "!!!"
#define HEADER_SYMBOL_LENGTH 3
#define HEADER_SYMBOL_LENGTH_NULL_TERMINATED HEADER_SYMBOL_LENGTH + 1
#define VARIABLE_ENCODED_TEXT_LENGTH 11

#define SAVE_GAME_VAR_NUM_START_SYMBOL 0x21 // Refers to ASCII exclamation mark. First visible character on the chart. To debug.

#define encode_stat(STAT) encode_integer(slot_data->p_player.stats[STAT].level)

typedef struct encode_integer_result {
  char txt[VARIABLE_ENCODED_TEXT_LENGTH];
}encode_integer_result;
typedef struct encode_string_result {
  char txt[VARIABLE_ENCODED_TEXT_LENGTH];
}encode_string_result;

typedef enum save_game_entry_order {
  SAVE_DATA_ORDER_START,
  SAVE_DATA_ORDER_CURRENCY_SOULS_PLAYER_HAVE,
  SAVE_DATA_STATS_HEALTH,
  SAVE_DATA_STATS_HP_REGEN,
  SAVE_DATA_STATS_MOVE_SPEED,
  SAVE_DATA_STATS_AOE,
  SAVE_DATA_STATS_DAMAGE,
  SAVE_DATA_STATS_ABILITY_CD,
  SAVE_DATA_STATS_PROJECTILE_AMOUTH,
  SAVE_DATA_STATS_EXP_GAIN,
  SAVE_DATA_STATS_TOTAL_TRAIT_POINTS,
  SAVE_DATA_ORDER_END,
}save_game_entry_order;

typedef struct save_game_system_state {
  std::vector<save_data> save_slots;
  u8 file_buffer[TOTAL_SAVE_FILE_SIZE];
  u8 variable_buffer[TOTAL_SAVE_FILE_SIZE];
  i32 file_datasize;
}save_game_system_state;

static save_game_system_state * state;

u32 save_game_get_entry(u32 offset);
encode_integer_result encode_integer(i32 val);
encode_string_result encode_string(const char* str, u16 len);
void parse_data(save_slots slot);
i32 decode_integer(void);

bool save_system_initialize(void) {
  if (state) {
    return true;
  }

  state = (save_game_system_state*)allocate_memory_linear(sizeof(save_game_system_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "save_game::save_system_initialize()::Save system state allocation failed");
    return false;
  }

  for (i32 itr_000 = 0; itr_000 < SAVE_SLOT_MAX; ++itr_000) {
    if (itr_000 == SAVE_SLOT_CURRENT_SESSION) {
      const char* file_name = TextFormat("%s%s","save_slot_current", SAVE_GAME_EXTENSION);
      state->save_slots.push_back(save_data(file_name));
    }
    else {
      const char* file_name = TextFormat("save_slot%d%s", itr_000+1, SAVE_GAME_EXTENSION);
      state->save_slots.push_back(save_data(file_name));
    }
  }

  return true;
}

bool parse_or_create_save_data_from_file(save_slots slot) {
  if (!state) {
    TraceLog(LOG_WARNING, "save_game::parse_or_create_save_data_from_file()::Save game state is not valid");
    return false;
  }
  if (!FileExists(state->save_slots.at(slot).file_name.c_str())) {
    return save_save_data(slot);
  }
  i32 out_datasize = 1;
  u8* data = LoadFileData(state->save_slots.at(slot).file_name.c_str(), &out_datasize);
  copy_memory(state->file_buffer, data, out_datasize);
  UnloadFileData(data);
  state->file_datasize = out_datasize;

  parse_data(slot);
  return true;
}

bool save_save_data(save_slots slot) { 
  if (!state) {
    TraceLog(LOG_WARNING, "save_game::save_save_data()::Save game state is not valid");
    return false;
  }
  save_data* slot_data = &state->save_slots.at(slot);
  encode_integer_result result_souls = encode_integer(slot_data->currency_souls_player_have);
  const char* slot_data_text = TextFormat("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
    result_souls.txt,
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_HEALTH).txt,
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_HP_REGEN).txt,
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_MOVE_SPEED).txt,
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_AOE).txt,
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_DAMAGE).txt,
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_ABILITY_CD).txt,
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_PROJECTILE_AMOUTH).txt,
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_EXP_GAIN).txt,
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_TOTAL_TRAIT_POINTS).txt,
    HEADER_SYMBOL_ENTRY
  );

  return SaveFileText(state->save_slots.at(slot).file_name.c_str(), (char*)slot_data_text);
}

void parse_data(save_slots slot) {
  save_game_entry_order entry_order = static_cast<save_game_entry_order>(save_game_entry_order::SAVE_DATA_ORDER_START+1);
  save_data* save_slot = &state->save_slots.at(slot);
  for (i32 i=0; i<state->file_datasize; ++i) {
    switch (entry_order) {
    case SAVE_DATA_ORDER_CURRENCY_SOULS_PLAYER_HAVE: {
      i = save_game_get_entry(i);
      i32 val = decode_integer();

      save_slot->currency_souls_player_have = val;
      break;
    }
    case SAVE_DATA_STATS_HEALTH: {
      i = save_game_get_entry(i);
      i32 val = decode_integer();

      save_slot->p_player.stats[CHARACTER_STATS_HEALTH].level = val;
      break; 
    }
    case SAVE_DATA_STATS_HP_REGEN: {
      i = save_game_get_entry(i);
      i32 val = decode_integer();

      save_slot->p_player.stats[CHARACTER_STATS_HP_REGEN].level = val;
      break; 
    }
    case SAVE_DATA_STATS_MOVE_SPEED: {
      i = save_game_get_entry(i);
      i32 val = decode_integer();

      save_slot->p_player.stats[CHARACTER_STATS_MOVE_SPEED].level = val;
      break; 
    }
    case SAVE_DATA_STATS_AOE: {
      i = save_game_get_entry(i);
      i32 val = decode_integer();

      save_slot->p_player.stats[CHARACTER_STATS_AOE].level = val;
      break; 
    }
    case SAVE_DATA_STATS_DAMAGE: {
      i = save_game_get_entry(i);
      i32 val = decode_integer();

      save_slot->p_player.stats[CHARACTER_STATS_DAMAGE].level = val;
      break; 
    }
    case SAVE_DATA_STATS_ABILITY_CD: {
      i = save_game_get_entry(i);
      i32 val = decode_integer();

      save_slot->p_player.stats[CHARACTER_STATS_ABILITY_CD].level = val;
      break; 
    }
    case SAVE_DATA_STATS_PROJECTILE_AMOUTH: {
      i = save_game_get_entry(i);
      i32 val = decode_integer();

      save_slot->p_player.stats[CHARACTER_STATS_PROJECTILE_AMOUTH].level = val;
      break; 
    }
    case SAVE_DATA_STATS_EXP_GAIN: {
      i = save_game_get_entry(i);
      i32 val = decode_integer();

      save_slot->p_player.stats[CHARACTER_STATS_EXP_GAIN].level = val;
      break; 
    }
    case SAVE_DATA_STATS_TOTAL_TRAIT_POINTS: {
      i = save_game_get_entry(i);
      i32 val = decode_integer();

      save_slot->p_player.stats[CHARACTER_STATS_TOTAL_TRAIT_POINTS].level = val;
      break; 
    }
    default: {
      TraceLog(LOG_ERROR, "pak_parser::parse_pak()::Unsupported reading order stage");
      return;
    }
    }
    entry_order = static_cast<save_game_entry_order>((entry_order % (SAVE_DATA_ORDER_END)) + 1);
    zero_memory(state->variable_buffer, sizeof(state->variable_buffer));
  }
  if (entry_order == SAVE_DATA_ORDER_END) {
    save_slot->is_success = true;
  }
}

save_data* get_save_data(save_slots slot) {
  if (!state) {
    TraceLog(LOG_ERROR, "save_game::get_save_data()::State is not valid");
    return 0;
  }
  return &state->save_slots.at(slot);
}
u32 save_game_get_entry(u32 offset) {
  u32 data_counter = 0;
  u32 out_offset = 0;

  for (u32 i=offset; i<TOTAL_SAVE_FILE_SIZE; ++i) {
    u8 header_symbol[HEADER_SYMBOL_LENGTH_NULL_TERMINATED] = {};
    copy_memory(header_symbol, state->file_buffer + i, HEADER_SYMBOL_LENGTH);

    if (TextIsEqual((const char*)header_symbol, HEADER_SYMBOL_ENTRY)) {
      break;
    }

    state->variable_buffer[data_counter] = state->file_buffer[i];
    out_offset = i;
    data_counter++;
  }

  return out_offset + HEADER_SYMBOL_LENGTH;
}
encode_integer_result encode_integer(i32 val) {
  encode_integer_result result = {};
  const char* souls_text = TextFormat("%d", val);
  i32 val_str_len = TextLength(souls_text);
  for (i32 i=0; i<val_str_len; ++i) {
    result.txt[i] = souls_text[i] + SAVE_GAME_VAR_NUM_START_SYMBOL;
  }

  return result;
}
encode_string_result encode_string(const char* str, u16 len) {
  encode_string_result result = {};

  copy_memory(result.txt, str, len);

  return result;
}
i32 decode_integer(void) {
  u32 var_len = TextLength((const char*)state->variable_buffer);
  char txt_val[VARIABLE_ENCODED_TEXT_LENGTH] = {};
  for (u32 i=0; i<var_len; ++i) {
    txt_val[i] = state->variable_buffer[i] - SAVE_GAME_VAR_NUM_START_SYMBOL;
  }

  return TextToInteger(txt_val);
}
