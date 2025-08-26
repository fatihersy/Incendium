#include "save_game.h"
#include "core/fmemory.h"
#include "core/logger.h"

#define TOTAL_SAVE_FILE_SIZE 2 * 1024 * 1024
#define HEADER_SYMBOL_ENTRY "!!!"
#define HEADER_SYMBOL_LENGTH 3
#define HEADER_SYMBOL_LENGTH_NULL_TERMINATED HEADER_SYMBOL_LENGTH + 1
#define VARIABLE_ENCODED_TEXT_LENGTH 11

#define SAVE_GAME_VAR_NUM_START_SYMBOL 0x21 // Refers to ASCII exclamation mark. First visible character on the chart. To debug.

#define encode_stat(STAT) encode_integer(slot_data->player_data.stats.at(STAT).current_level)

typedef struct encode_integer_result {
  std::string str = std::string("");

  encode_integer_result(void) {
    this->str = std::string("");
  }
}encode_integer_result;

typedef struct encode_string_result {
  std::string str = std::string("");

  encode_string_result(void) {
    this->str = std::string("");
  }
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
  std::array<save_data, SAVE_SLOT_MAX> save_slots;
  std::array<std::string, SAVE_SLOT_MAX> slot_filenames;
  std::string file_buffer;
  std::string variable_buffer;
  i32 file_datasize;

  save_game_system_state(void) {
    this->save_slots.fill(save_data());
    this->slot_filenames.fill(std::string(""));
    this->file_buffer = std::string("");
    this->variable_buffer = std::string("");
    this->file_datasize = 0;
  } 
}save_game_system_state;

static save_game_system_state * state = nullptr;

size_t save_game_get_entry(size_t offset);
encode_integer_result encode_integer(i32 val);
encode_string_result encode_string(const char* str, i32 len);
void parse_data(save_slot_id slot);
i32 decode_integer(void);
void setup_save_data(save_data* data);
std::string get_save_filename(save_slot_id slot);

bool save_system_initialize(void) {
  if (state and state != nullptr) {
    return true;
  }

  state = (save_game_system_state*)allocate_memory_linear(sizeof(save_game_system_state), true);
  if (not state or state == nullptr) {
    IFATAL("save_game::save_system_initialize()::Save system state allocation failed");
    return false;
  }
  *state = save_game_system_state();

  for (size_t itr_000 = 0; itr_000 < SAVE_SLOT_MAX; ++itr_000) {
    state->slot_filenames.at(itr_000) = get_save_filename(static_cast<save_slot_id>(itr_000));
  }

  return true;
}

bool parse_or_create_save_data_from_file(save_slot_id slot, save_data default_save) {
  if (not state or state == nullptr) {
    IFATAL("save_game::parse_or_create_save_data_from_file()::Save game state is not valid");
    return false;
  }
  if (not FileExists(state->slot_filenames.at(slot).c_str())) {
    setup_save_data(__builtin_addressof(default_save));
    state->save_slots.at(slot) = default_save;
    return save_save_data(slot);
  }
  i32 out_datasize = 1;
  u8* data = LoadFileData(state->slot_filenames.at(slot).c_str(), __builtin_addressof(out_datasize));
  state->file_buffer.assign(data, data + out_datasize);
  UnloadFileData(data);
  state->file_datasize = out_datasize;

  parse_data(slot);
  return true;
}

bool save_save_data(save_slot_id slot) { 
  if (not state or state == nullptr) {
    IERROR("save_game::save_save_data()::Save game state is not valid");
    return false;
  }
  if (slot < SAVE_SLOT_UNDEFINED or slot >= SAVE_SLOT_MAX) {
    IWARN("save_game::save_save_data()::Slot out of bound");
    return false;
  }

  save_data* slot_data = __builtin_addressof(state->save_slots.at(slot));
  encode_integer_result result_souls = encode_integer(slot_data->currency_souls_player_have);
  const char* slot_data_text = TextFormat("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
    result_souls.str.c_str(),
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_HEALTH).str.c_str(),
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_HP_REGEN).str.c_str(),
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_MOVE_SPEED).str.c_str(),
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_AOE).str.c_str(),
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_DAMAGE).str.c_str(),
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_ABILITY_CD).str.c_str(),
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_PROJECTILE_AMOUTH).str.c_str(),
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_EXP_GAIN).str.c_str(),
    HEADER_SYMBOL_ENTRY,
    encode_stat(CHARACTER_STATS_TOTAL_TRAIT_POINTS).str.c_str(),
    HEADER_SYMBOL_ENTRY
  );

  return SaveFileText(state->slot_filenames.at(slot).c_str(), (char*)slot_data_text);
}
bool does_save_exist(save_slot_id slot) {
  if (not state or state == nullptr) {
    IERROR("save_game::does_save_exist()::State is not valid");
    return false;
  }
  if (slot < SAVE_SLOT_UNDEFINED or slot >= SAVE_SLOT_MAX) {
    IWARN("save_game::does_save_exist()::Slot out of bound");
    return false;
  }

  return FileExists(state->slot_filenames.at(slot).c_str());
}

void parse_data(save_slot_id slot) {
  if (not state or state == nullptr) {
    IERROR("save_game::parse_data()::State is invalid");
    return;
  }
  if (slot < SAVE_SLOT_UNDEFINED or slot >= SAVE_SLOT_MAX) {
    IWARN("save_game::parse_data()::Slot out of bound");
    return;
  }

  save_game_entry_order entry_order = static_cast<save_game_entry_order>(save_game_entry_order::SAVE_DATA_ORDER_START+1);
  save_data* save_slot = __builtin_addressof(state->save_slots.at(slot));
  for (i32 itr_000 = 0; itr_000 < state->file_datasize; ++itr_000) {
    switch (entry_order) {
    case SAVE_DATA_ORDER_CURRENCY_SOULS_PLAYER_HAVE: {
      itr_000 = save_game_get_entry(itr_000);
      i32 val = decode_integer();

      save_slot->currency_souls_player_have = val;
      break;
    }
    case SAVE_DATA_STATS_HEALTH: {
      itr_000 = save_game_get_entry(itr_000);
      i32 val = decode_integer();

      save_slot->player_data.stats.at(CHARACTER_STATS_HEALTH).current_level = val;
      break; 
    }
    case SAVE_DATA_STATS_HP_REGEN: {
      itr_000 = save_game_get_entry(itr_000);
      i32 val = decode_integer();

      save_slot->player_data.stats.at(CHARACTER_STATS_HP_REGEN).current_level = val;
      break; 
    }
    case SAVE_DATA_STATS_MOVE_SPEED: {
      itr_000 = save_game_get_entry(itr_000);
      i32 val = decode_integer();

      save_slot->player_data.stats.at(CHARACTER_STATS_MOVE_SPEED).current_level = val;
      break; 
    }
    case SAVE_DATA_STATS_AOE: {
      itr_000 = save_game_get_entry(itr_000);
      i32 val = decode_integer();

      save_slot->player_data.stats.at(CHARACTER_STATS_AOE).current_level = val;
      break; 
    }
    case SAVE_DATA_STATS_DAMAGE: {
      itr_000 = save_game_get_entry(itr_000);
      i32 val = decode_integer();

      save_slot->player_data.stats.at(CHARACTER_STATS_DAMAGE).current_level = val;
      break; 
    }
    case SAVE_DATA_STATS_ABILITY_CD: {
      itr_000 = save_game_get_entry(itr_000);
      i32 val = decode_integer();

      save_slot->player_data.stats.at(CHARACTER_STATS_ABILITY_CD).current_level = val;
      break; 
    }
    case SAVE_DATA_STATS_PROJECTILE_AMOUTH: {
      itr_000 = save_game_get_entry(itr_000);
      i32 val = decode_integer();

      save_slot->player_data.stats.at(CHARACTER_STATS_PROJECTILE_AMOUTH).current_level = val;
      break; 
    }
    case SAVE_DATA_STATS_EXP_GAIN: {
      itr_000 = save_game_get_entry(itr_000);
      i32 val = decode_integer();

      save_slot->player_data.stats.at(CHARACTER_STATS_EXP_GAIN).current_level = val;
      break; 
    }
    case SAVE_DATA_STATS_TOTAL_TRAIT_POINTS: {
      itr_000 = save_game_get_entry(itr_000);
      i32 val = decode_integer();

      save_slot->player_data.stats.at(CHARACTER_STATS_TOTAL_TRAIT_POINTS).current_level = val;
      break; 
    }
    default: {
      IERROR("save_game::parse_data()::Unsupported reading order stage");
      return;
    }
    }
    entry_order = static_cast<save_game_entry_order>((entry_order % (SAVE_DATA_ORDER_END)) + 1);
    state->variable_buffer.clear();
  }
  if (entry_order == SAVE_DATA_ORDER_END) {
    save_slot->is_success = true;
  }
}

save_data* get_save_data(save_slot_id slot) {
  if (not state or state == nullptr) {
    IERROR("save_game::get_save_data()::State is not valid");
    return nullptr;
  }
  if (slot < SAVE_SLOT_UNDEFINED or slot >= SAVE_SLOT_MAX) {
    IWARN("save_game::get_save_data()::Slot out of bound");
    return nullptr;
  }

  return __builtin_addressof(state->save_slots.at(slot));
}
size_t save_game_get_entry(size_t offset) {
  if (not state or state == nullptr) {
    IERROR("save_game::save_game_get_entry()::State is not valid");
    return 0u;
  }
  size_t out_offset = 0;

  for (size_t itr_000 = offset; itr_000 < TOTAL_SAVE_FILE_SIZE; ++itr_000) {
    std::string header_symbol = std::string("");
    header_symbol.assign(state->file_buffer.begin() + itr_000, state->file_buffer.begin() + itr_000 + HEADER_SYMBOL_LENGTH);

    if (TextIsEqual(header_symbol.c_str(), HEADER_SYMBOL_ENTRY)) {
      break;
    }

    state->variable_buffer.push_back(state->file_buffer.at(itr_000));
    out_offset = itr_000;
  }

  return out_offset + HEADER_SYMBOL_LENGTH;
}
void setup_save_data(save_data* data) {
  if (not state or state == nullptr) {
    IERROR("save_game::setup_save_data()::Slot out of bound");
    return;
  }
  if (not data or data == nullptr) {
    IWARN("save_game::setup_save_data()::Slot out of bound");
    return;
  }
  if (data->id < SAVE_SLOT_UNDEFINED or data->id >= SAVE_SLOT_MAX) {
    IWARN("save_game::setup_save_data()::Slot out of bound");
    return;
  }

  data->file_name = state->slot_filenames.at(data->id);
}
std::string get_save_filename(save_slot_id slot) {
  if (slot < SAVE_SLOT_UNDEFINED or slot >= SAVE_SLOT_MAX) {
    IWARN("save_game::get_save_filename()::Slot out of bound");
    return std::string("");
  }
  if (slot == SAVE_SLOT_CURRENT_SESSION) {
    return std::string(TextFormat("%s%s","save_slot_current", SAVE_GAME_EXTENSION));
  }
  else {
    return std::string(TextFormat("save_slot%d%s", static_cast<i32>(slot)+1, SAVE_GAME_EXTENSION));
  }
}
encode_integer_result encode_integer(i32 val) {
  encode_integer_result result = encode_integer_result();
  std::string souls_text = TextFormat("%d", val);

  for (size_t itr_000 = 0; itr_000 < souls_text.size(); ++itr_000) {
    result.str.push_back(souls_text.at(itr_000) + SAVE_GAME_VAR_NUM_START_SYMBOL);
  }
  return result;
}
encode_string_result encode_string(const char* str, [[__maybe_unused__]] i32 len) {
  encode_string_result result = encode_string_result();
  result.str = str;
  return result;
}
i32 decode_integer(void) {
  i32 var_len = TextLength(state->variable_buffer.c_str());
  std::string txt_val = std::string("");
  for (i32 itr_000 = 0; itr_000 < var_len; ++itr_000) {
    txt_val.push_back(state->variable_buffer.at(itr_000) - SAVE_GAME_VAR_NUM_START_SYMBOL);
  }

  return TextToInteger(txt_val.c_str());
}
