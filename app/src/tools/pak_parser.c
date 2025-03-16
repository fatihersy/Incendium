#include "pak_parser.h"
#include <defines.h>

#include "core/fmemory.h"

#define TOTAL_PAK_FILE_SIZE 64 * 1024 * 1024

#define MAX_RESOURCE_FILES 256

#define MAX_FILENAME_LENGTH 64
#define MAX_FILENAME_EXT_LENGTH 5
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

typedef struct file_data {
  i8 file_name[MAX_FILENAME_LENGTH];
  i8 file_extension[MAX_FILENAME_EXT_LENGTH];
  u64 size;
  u32 offset;
}file_data;

typedef struct pak_parser_system_state {
  file_data file_datas[MAX_RESOURCE_FILES];

  u8 pak_data[TOTAL_PAK_FILE_SIZE];
  u8 read_buffer[TOTAL_PAK_FILE_SIZE];
  u32 pak_data_size;
  u32 pak_data_offset;
  u32 total_file_count;

  u32 header_symbol_between_len;
  u32 header_symbol_data_start_len;
  u32 header_symbol_data_end_len;
}pak_parser_system_state;

static pak_parser_system_state *restrict state;

u32 get_entry(u32 offset);

void pak_parser_system_initialize(void) {
  if (state) {
    TraceLog(LOG_WARNING, "pak_parser::pak_parser_system_initialize()::Init function called twice");
    return;
  }

  state = (pak_parser_system_state*)allocate_memory_linear(sizeof(pak_parser_system_state), true);

  state->header_symbol_between_len = TextLength(HEADER_SYMBOL_ENTRY);
}

void parse_pak(const char* path) {
  if (!FileExists(path)) {
    TraceLog(LOG_WARNING, "pak_parser::parse_pak()::Path does not exist");
    return;
  }
  i32 data_size = 1;
  u8 * data = LoadFileData(path, &data_size);
  copy_memory(state->pak_data, data, data_size);
  UnloadFileData(data);

  state->pak_data_size = data_size;

  pak_reading_order reading_order = READING_ORDER_FILENAME;
  file_data file = {0};
  for (u32 i=0; i<state->pak_data_size; ++i) {
    switch (reading_order) {
    case READING_ORDER_FILENAME: {
      i = get_entry(i);
      copy_memory(file.file_name, state->read_buffer, TextLength((const char*)state->read_buffer));
      break;
    }
    case READING_ORDER_FILE_EXTENSION: {
      i = get_entry(i);
      copy_memory(file.file_extension, state->read_buffer, TextLength((const char*)state->read_buffer));
      break;
    }
    case READING_ORDER_SIZE: {
      i = get_entry(i);
      file.size = TextToInteger((const char*)state->read_buffer);
      break;
    }
    case READING_ORDER_DATA_LOCATION_OFFSET: {
      file.offset = i;
      i = get_entry(i);
      break;
    }
    default: {
      TraceLog(LOG_ERROR, "pak_parser::parse_pak()::Unsupported reading order stage");
      return;
    }
    }
    reading_order = (reading_order % (READING_ORDER_MAX)) + 1;
    if (reading_order == READING_ORDER_MAX) {
      state->file_datas[state->total_file_count] = file;
      file = (file_data){0};
      state->total_file_count++;
      reading_order = (reading_order % (READING_ORDER_MAX)) + 1;
    }
    
    zero_memory(state->read_buffer, sizeof(state->read_buffer));
  }

}
 
u32 get_entry(u32 offset) {

  u8 seek_header[HEADER_SYMBOL_LENGTH_NULL_TERMINATED] = {0};
  u32 data_counter = 0;
  u32 out_offset = 0;

  for (u32 i=offset; i<state->pak_data_size; ++i) {
    if (TextIsEqual((const char*)seek_header, HEADER_SYMBOL_ENTRY)) {
      break;
    }
    if (data_counter < HEADER_SYMBOL_LENGTH) {
      seek_header[data_counter] = state->pak_data[i];
    } else {      
      for (i32 j=0; j<HEADER_SYMBOL_LENGTH-1; ++j) { // Rotate header
        seek_header[j] = seek_header[j+1];
      }
      seek_header[HEADER_SYMBOL_LENGTH-1] = state->pak_data[i];
    }

    state->read_buffer[data_counter] = state->pak_data[i];
    
    out_offset = i;
    data_counter++;
  }
  zero_memory(state->read_buffer + data_counter - HEADER_SYMBOL_LENGTH, HEADER_SYMBOL_LENGTH);

  return out_offset;
}
