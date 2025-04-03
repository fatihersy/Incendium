#include "save_game.h"
#include "core/fmemory.h"
#include "raylib.h"

#define TOTAL_SAVE_FILE_SIZE 2 * 1024 * 1024
#define HEADER_SYMBOL_ENTRY "!!!"
#define HEADER_SYMBOL_LENGTH 3
#define HEADER_SYMBOL_LENGTH_NULL_TERMINATED HEADER_SYMBOL_LENGTH + 1
#define VARIABLE_ENCODED_TEXT_LENGTH 11

#define SAVE_GAME_VAR_NUM_START_SYMBOL 0x21 // Refers to ASCII exclamation mark. First visible character on the chart. To debug.

typedef struct encode_integer_result {
  char txt[VARIABLE_ENCODED_TEXT_LENGTH];
}encode_integer_result;
typedef struct encode_string_result {
  char txt[VARIABLE_ENCODED_TEXT_LENGTH];
}encode_string_result;

typedef enum save_game_entry_order {
  SAVE_DATA_ORDER_START,
  SAVE_DATA_ORDER_CURRENCY_SOULS_PLAYER_HAVE,
  SAVE_DATA_ORDER2,
  SAVE_DATA_ORDER_END,
}save_game_entry_order;

typedef struct save_game_system_state {
  save_data save_slots[SAVE_SLOT_MAX];
  u8 file_buffer[TOTAL_SAVE_FILE_SIZE];
  u8 variable_buffer[TOTAL_SAVE_FILE_SIZE];
  i32 file_datasize;
}save_game_system_state;

static save_game_system_state *restrict state;

u32 save_game_get_entry(u32 offset);
encode_integer_result encode_integer(i32 val);
encode_string_result encode_string(const char* str, u16 len);
void parse_data(save_slots slot);
i32 decode_integer(void);

void save_system_initialize(void) {
  if (state) {
    TraceLog(LOG_WARNING, "save_game::save_system_initialize()::Init called twice");
    return;
  }

  state = (save_game_system_state*)allocate_memory_linear(sizeof(save_game_system_state), true);

  for (i32 i=0; i<SAVE_SLOT_MAX; ++i) {
    if (SAVE_DATA_ORDER_START + i == SAVE_SLOT_CURRENT_SESSION) {
      const char* file_name = TextFormat("%s%s","save_slot_current", SAVE_GAME_EXTENSION);
      copy_memory(state->save_slots[i].file_name, file_name, TextLength(file_name));
    }
    else {
      const char* file_name = TextFormat("save_slot%d%s", i+1, SAVE_GAME_EXTENSION);
      copy_memory(state->save_slots[i].file_name, file_name, TextLength(file_name));
    }
  }
}

bool parse_or_create_save_data_from_file(save_slots slot) {
  if (!state) {
    TraceLog(LOG_WARNING, "save_game::parse_or_create_save_data_from_file()::Save game state is not valid");
    return false;
  }
  if (!FileExists((const char*)state->save_slots[slot].file_name)) {
    return save_save_data(slot);
  }
  i32 out_datasize;
  u8* data = LoadFileData((const char*)state->save_slots[slot].file_name, &out_datasize);
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
  save_data* slot_data = &state->save_slots[slot];
  encode_integer_result result = encode_integer(slot_data->currency_souls_player_have);
  const char* slot_data_text = TextFormat("%s%s",
    result.txt,
    HEADER_SYMBOL_ENTRY
  );

  return SaveFileText((const char*)state->save_slots[slot].file_name, (char*)slot_data_text);
}

void parse_data(save_slots slot) {

  save_game_entry_order entry_order = SAVE_DATA_ORDER_START+1;
  save_data* save_slot = &state->save_slots[slot];
  for (i32 i=0; i<state->file_datasize; ++i) {
    switch (entry_order) {
    case SAVE_DATA_ORDER_START: { break; }
    case SAVE_DATA_ORDER_CURRENCY_SOULS_PLAYER_HAVE: {
      i = save_game_get_entry(i);
      u32 val = decode_integer();

      save_slot->currency_souls_player_have = val;
      break;
    }
    case SAVE_DATA_ORDER2: { break; }
    case SAVE_DATA_ORDER_END: { break;}
    default: {
      TraceLog(LOG_ERROR, "pak_parser::parse_pak()::Unsupported reading order stage");
      return;
    }
    }
    entry_order = (entry_order % (SAVE_DATA_ORDER_END)) + 1;
    zero_memory(state->variable_buffer, sizeof(state->variable_buffer));
  }
}

save_data* get_save_data(save_slots slot) {
  if (!state) {
    TraceLog(LOG_ERROR, "save_game::get_save_data()::State is not valid");
    return 0;
  }
  return &state->save_slots[slot];
}
u32 save_game_get_entry(u32 offset) {
  u32 data_counter = 0;
  u32 out_offset = 0;

  for (u32 i=offset; i<TOTAL_SAVE_FILE_SIZE; ++i) {
    u8 header_symbol[HEADER_SYMBOL_LENGTH_NULL_TERMINATED] = {0};
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
  encode_integer_result result = {0};
  const char* souls_text = TextFormat("%d", val);
  i32 val_str_len = TextLength(souls_text);
  for (i32 i=0; i<val_str_len; ++i) {
    result.txt[i] = souls_text[i] + SAVE_GAME_VAR_NUM_START_SYMBOL;
  }

  return result;
}
encode_string_result encode_string(const char* str, u16 len) {
  encode_string_result result = {0};

  copy_memory(result.txt, str, len);

  return result;
}
i32 decode_integer(void) {
  u32 var_len = TextLength((const char*)state->variable_buffer);
  char txt_val[VARIABLE_ENCODED_TEXT_LENGTH] = {0};
  for (u32 i=0; i<var_len; ++i) {
    txt_val[i] = state->variable_buffer[i] - SAVE_GAME_VAR_NUM_START_SYMBOL;
  }

  return TextToInteger(txt_val);
}
