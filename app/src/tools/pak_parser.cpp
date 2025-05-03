#include "pak_parser.h"
#include "core/fmemory.h"

#include "raylib.h"

#define TOTAL_PAK_FILE_SIZE 128 * 1024 * 1024
#define MAX_RESOURCE_FILES 256

#define MAX_FILE_ENTRY_LENGTH 64

#define HEADER_SYMBOL_ENTRY "__ENTRY__"
#define HEADER_SYMBOL_LENGTH 9
#define HEADER_SYMBOL_LENGTH_NULL_TERMINATED HEADER_SYMBOL_LENGTH + 1

typedef enum pak_reading_order {
  READING_ORDER_UNDEFINED,
  READING_ORDER_FILENAME,
  READING_ORDER_FILE_EXTENSION,
  READING_ORDER_SIZE,
  READING_ORDER_DATA_LOCATION_OFFSET,
  READING_ORDER_MAX,
} pak_reading_order;

typedef enum file_extension {
  FILE_EXT_UNDEFINED,
  FILE_EXT_PNG,
  FILE_EXT_TXT,
  FILE_EXT_TTF,
  FILE_EXT_MAX
}file_extension;

typedef struct filename_offset_data {
  u32 filename_offset;
  u32 path_length;

  bool is_success;
}filename_offset_data;

typedef struct pak_parser_system_state {
  file_data file_datas[MAX_RESOURCE_FILES];

  u8 pak_data[TOTAL_PAK_FILE_SIZE];
  u8 read_buffer[TOTAL_PAK_FILE_SIZE];
  u32 pak_data_size;
  u32 pak_data_offset;
  u32 total_file_count;
}pak_parser_system_state;

static pak_parser_system_state * state;

u32 pak_parser_get_entry(u32 offset);

void pak_parser_system_initialize(const char* path) {
  if (state) {
    TraceLog(LOG_WARNING, "pak_parser::pak_parser_system_initialize()::Init function called twice");
    return;
  }
  if (!FileExists(path)) {
    TraceLog(LOG_WARNING, "pak_parser::parse_pak()::Path does not exist");
    return;
  }
  state = (pak_parser_system_state*)allocate_memory_linear(sizeof(pak_parser_system_state), true);

  i32 data_size = 1;
  u8 * data = LoadFileData(path, &data_size);
  copy_memory(state->pak_data, data, data_size);
  UnloadFileData(data);
  state->pak_data_size = data_size;
}

void parse_pak(void) {
  if (!state) {
    TraceLog(LOG_ERROR, "pak_parser::parse_pak()::Pak parser system didn't initialized");
    return;
  }
  pak_reading_order reading_order = READING_ORDER_FILENAME;
  file_data file = {};
  for (u32 i=0; i<state->pak_data_size; ++i) {
    switch (reading_order) {
    case READING_ORDER_FILENAME: {
      i = pak_parser_get_entry(i);
      copy_memory(file.file_name, state->read_buffer, TextLength((const char*)state->read_buffer));
      break;
    }
    case READING_ORDER_FILE_EXTENSION: {
      i = pak_parser_get_entry(i);
      copy_memory(file.file_extension, state->read_buffer, TextLength((const char*)state->read_buffer));
      break;
    }
    case READING_ORDER_SIZE: {
      i = pak_parser_get_entry(i);
      file.size = TextToInteger((const char*)state->read_buffer);
      break;
    }
    case READING_ORDER_DATA_LOCATION_OFFSET: {
      file.offset = i;
      file.data = &state->pak_data[i];
      i = pak_parser_get_entry(i);
      break;
    }
    default: {
      TraceLog(LOG_ERROR, "pak_parser::parse_pak()::Unsupported reading order stage");
      return;
    }
    }
    reading_order = static_cast<pak_reading_order>((reading_order % (pak_reading_order::READING_ORDER_MAX)) + 1);
    if (reading_order == pak_reading_order::READING_ORDER_MAX) {
      file.is_initialized = true;
      state->file_datas[state->total_file_count] = file;
      file = file_data {};
      state->total_file_count++;
      reading_order = static_cast<pak_reading_order>((reading_order % (pak_reading_order::READING_ORDER_MAX)) + 1);
    }
    
    zero_memory(state->read_buffer, sizeof(state->read_buffer));
  }

}
 
u32 pak_parser_get_entry(u32 offset) {
  u32 data_counter = 0;
  u32 out_offset = 0;

  for (u32 i=offset; i<state->pak_data_size; ++i) {

    u8 header_symbol[HEADER_SYMBOL_LENGTH_NULL_TERMINATED] = {};
    copy_memory(header_symbol, state->pak_data + i, HEADER_SYMBOL_LENGTH);

    if (TextIsEqual((const char*)header_symbol, HEADER_SYMBOL_ENTRY)) {
      break;
    }

    state->read_buffer[data_counter] = state->pak_data[i];
    
    out_offset = i;
    data_counter++;
  }
  return out_offset + HEADER_SYMBOL_LENGTH;
}

file_data get_file_data(const char* file_name) {
  if (!state) {
    TraceLog(LOG_ERROR, "pak_parser::get_file_data()::Pak parser system didn't initialized");
    return file_data {};
  }

  for (u32 i=0; i<state->total_file_count; ++i) {
    if(TextIsEqual((const char*)state->file_datas[i].file_name, file_name)) {
      return state->file_datas[i];
    }
  }

  return file_data {};
}

/**
 * @brief get_file_data() returns readed data unlike the fetch_file_data() reads when it's called
 */
file_data fetch_file_data(const char* file_name) {
  if (!state) {
    TraceLog(LOG_ERROR, "pak_parser::fetch_file_data()::Pak parser system didn't initialized");
    return file_data{};
  }
  pak_reading_order reading_order = READING_ORDER_FILENAME;
  file_data file = {};
  bool found = false;
  for (u32 i=0; i<state->pak_data_size; ++i) {
    switch (reading_order) {
    case READING_ORDER_FILENAME: {
      i = pak_parser_get_entry(i);
      if (TextIsEqual((const char*)state->read_buffer, file_name)) {
        found = true;
        copy_memory(file.file_name, state->read_buffer, TextLength((const char*)state->read_buffer));
      }
      break;
    }
    case READING_ORDER_FILE_EXTENSION: {
      i = pak_parser_get_entry(i);
      if (found) {
        copy_memory(file.file_extension, state->read_buffer, TextLength((const char*)state->read_buffer));
      }
      break;
    }
    case READING_ORDER_SIZE: {
      i = pak_parser_get_entry(i);
      if (found) {
        file.size = TextToInteger((const char*)state->read_buffer);
      }
      break;
    }
    case READING_ORDER_DATA_LOCATION_OFFSET: {
      if (found) {
        file.offset = i;
        file.data = &state->pak_data[i];
      }
      i = pak_parser_get_entry(i);
      break;
    }
    default: {
      TraceLog(LOG_ERROR, "pak_parser::parse_pak()::Unsupported reading order stage");
      return file_data{};
    }
    }
    reading_order = static_cast<pak_reading_order>((reading_order % (READING_ORDER_MAX)) + 1);
    if (reading_order == READING_ORDER_MAX && found) {
      break;
    }
    zero_memory(state->read_buffer, sizeof(state->read_buffer));
  }

  return file;
}
