#include "pak_parser.h"
#include "core/fmemory.h"

#include "raylib.h"

#define MAX_RESOURCE_FILES 32

#define MAX_FILE_ENTRY_LENGTH 64

#define HEADER_SYMBOL_BEGIN "__BEGIN__"
#define HEADER_SYMBOL_BEGIN_LENGTH 9
#define HEADER_SYMBOL_END "__END__"
#define HEADER_SYMBOL_END_LENGTH 7

#define HEADER_SYMBOL_LENGTH_NULL_TERMINATED HEADER_SYMBOL_LENGTH + 1

typedef enum map_pak_reading_order {
  MAP_PAK_READING_ORDER_UNDEFINED,
  MAP_PAK_READING_ORDER_MAX,
} map_pak_reading_order;

typedef enum file_extension {
  FILE_EXT_UNDEFINED,
  FILE_EXT_PNG,
  FILE_EXT_TXT,
  FILE_EXT_TTF,
  FILE_EXT_MAX
}file_extension;

typedef struct filename_offset_data {
  size_t filename_offset;
  size_t path_length;

  bool is_success;
  filename_offset_data(void) {
    this->filename_offset = 0u;
    this->path_length = 0u;
    this->is_success = false;
  }
}filename_offset_data;

typedef struct pak_parser_system_state {
  std::array<worldmap_stage_file, MAX_WORLDMAP_LOCATIONS> worldmap_location_file_datas;

  std::vector<asset_pak_file> asset_pak_datas;
  std::string map_pak_data;
  bool is_map_pak_data_initialized;

  std::string read_buffer;
  pak_parser_system_state(void) {
    this->worldmap_location_file_datas.fill(worldmap_stage_file());

    this->asset_pak_datas = std::vector<asset_pak_file>();
    this->map_pak_data = std::string("");
    this->is_map_pak_data_initialized = false;

    this->read_buffer = std::string("");
  }
} pak_parser_system_state;

static pak_parser_system_state * state = nullptr;
std::string read_asset_file_data(pak_file_id pak_id, i32 index, size_t offset);
worldmap_stage_file read_map_file_data(i32 index, size_t offset);

size_t pak_parser_read_to_begin(std::string::const_iterator iter, size_t offset, size_t delimiter);
size_t pak_parser_read_to_end(std::string::const_iterator iter, size_t offset, size_t delimiter);

std::string pak_id_to_file_name(pak_file_id id);
std::string * pak_id_to_pak_data_pointer(pak_file_id id);
file_buffer * pak_id_to_file_data_pointer(pak_file_id id, i32 index);
void assign_pak_data_by_id(pak_file_id id, std::string data);

i32 read_file(const char * path) {
  if (not FileExists(path)) {
    TraceLog(LOG_INFO, "main::write_map_pak()::file '%s' doesn't exist", path);
    return 0;
  }
  state->read_buffer.clear();
  i32 loaded_data = 1;
  u8* data = LoadFileData(path, &loaded_data);
  if (loaded_data <= 1 ) {
    TraceLog(LOG_INFO, "main::write_map_pak()::file '%s' doesn't exist", path);
    return 0;
  }
  state->read_buffer.append(data, data + loaded_data);
  UnloadFileData(data);
	return loaded_data;
}

bool pak_parser_system_initialize(void) {
  if (state and state != nullptr) {
    TraceLog(LOG_WARNING, "pak_parser::pak_parser_system_initialize()::Called twice");
    return true;
  }
  state = (pak_parser_system_state*)allocate_memory_linear(sizeof(pak_parser_system_state), true);
  if (not state or state == nullptr) {
    TraceLog(LOG_WARNING, "pak_parser::pak_parser_system_initialize()::State allocation failed");
    return false;
  }
  *state = pak_parser_system_state();

  state->asset_pak_datas.push_back(asset_pak_file("asset1.pak", "asset1_files/",
    std::vector<file_buffer>({
      file_buffer(PAK_FILE_ASSET1, PAK_FILE_ASSET1_THUMBNAIL, ".png"),
      file_buffer(PAK_FILE_ASSET1, PAK_FILE_ASSET1_FONT_ABRACADABRA, ".ttf"),
      file_buffer(PAK_FILE_ASSET1, PAK_FILE_ASSET1_FONT_MIOSEVKA, ".ttf"),
      file_buffer(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_BTN_CLICK_1, ".wav"),
      file_buffer(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_BTN_CLICK_2, ".wav"),
      file_buffer(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_BTN_CLICK_3, ".wav"),
      file_buffer(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_DENY, ".wav"),
      file_buffer(PAK_FILE_ASSET1, PAK_FILE_ASSET1_MUSIC_MAIN_MENU_THEME, ".wav"),
      file_buffer(PAK_FILE_ASSET1, PAK_FILE_ASSET1_MUSIC_NIGHT_THEME, ".wav"),
      file_buffer(PAK_FILE_ASSET1, PAK_FILE_ASSET1_MUSIC_TRACK_5, ".wav"),
      file_buffer(PAK_FILE_ASSET1, PAK_FILE_ASSET1_WORLDMAP_IMAGE, ".png"),
    })
  ));
  state->asset_pak_datas.push_back(asset_pak_file("asset2.pak", "asset2_files/",
    std::vector<file_buffer>({
      file_buffer(PAK_FILE_ASSET2, PAK_FILE_ASSET2_FADE_TRANSITION, "fs"),
      file_buffer(PAK_FILE_ASSET2, PAK_FILE_ASSET2_FONT_OUTLINE, "fs"),
      file_buffer(PAK_FILE_ASSET2, PAK_FILE_ASSET2_MAP_CHOICE_IMAGE, "fs"),
      file_buffer(PAK_FILE_ASSET2, PAK_FILE_ASSET2_POST_PROCESS, "fs"),
      file_buffer(PAK_FILE_ASSET2, PAK_FILE_ASSET2_PRG_BAR_MASK, "fs"),
      file_buffer(PAK_FILE_ASSET2, PAK_FILE_ASSET2_ATLAS, "png"),
    })
  ));

  return true;
}

bool parse_pak(pak_file_id id) {
  if (not state or state == nullptr) {
    TraceLog(LOG_ERROR, "pak_parser::parse_pak()::State is invalid");
    return false;
  }
  const std::string path = pak_id_to_file_name(id);
  state->read_buffer.clear();

  if (not read_file(path.c_str())) {
    TraceLog(LOG_ERROR, "pak_parser::parse_pak()::File read failed");
    return false;
  }
  assign_pak_data_by_id(id, state->read_buffer.data());

  return true;
}

const file_buffer * get_asset_file_buffer(pak_file_id id, i32 index) {
  if (not state or state == nullptr) {
    TraceLog(LOG_ERROR, "pak_parser::get_asset_file_buffer()::Pak parser system didn't initialized");
    return nullptr;
  }
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    TraceLog(LOG_ERROR, "pak_parser::get_asset_file_buffer()::File id is out of bound");
    return nullptr;
  }
  switch (id) {
    case PAK_FILE_ASSET1:{
      file_buffer& file = state->asset_pak_datas.at(0).file_buffers.at(index-1);
      if (file.is_success) {
        return &file;
      }
      else {
        return fetch_asset_file_buffer(id, index);
      }
    }
    case PAK_FILE_ASSET2:{
      file_buffer& file = state->asset_pak_datas.at(1).file_buffers.at(index-1);
      if (file.is_success) {
        return &file;
      }
      else {
        return fetch_asset_file_buffer(id, index);
      }
    }
    default:{
      TraceLog(LOG_ERROR, "pak_parser::get_asset_file_buffer()::Unsupported pak id");
      return nullptr;
    }
  }
  TraceLog(LOG_ERROR, "pak_parser::get_asset_file_buffer()::Function ended unexpectedly");
  return nullptr;
}
worldmap_stage_file get_map_file_data(i32 index) {
  if (not state or state == nullptr) {
    TraceLog(LOG_ERROR, "pak_parser::get_map_file_data()::Pak parser system didn't initialized");
    return worldmap_stage_file();
  }
  if (state->worldmap_location_file_datas.at(index).is_success) {
    return state->worldmap_location_file_datas.at(index);
  }
  else {
    return fetch_map_file_data(PAK_FILE_MAP, index);
  }
}

/**
 * @brief get_map_file_data() returns readed data unlike the fetch_file_data() reads when it's called
 */

std::string read_asset_file_data(pak_file_id pak_id, i32 index, size_t offset) {
  if (index < 0 or index > MAX_RESOURCE_FILES) {
    TraceLog(LOG_ERROR, "pak_parser::read_asset_file_data()::File index is out of bound");
    return std::string();
  }
  std::string * pak_data = pak_id_to_pak_data_pointer(pak_id);
  std::string::const_iterator itr = pak_data->begin() + offset;
  state->read_buffer.clear();

  for (i32 itr_000 = 1; itr_000 < static_cast<i32>(pak_data->size()); ++itr_000) 
	{
		offset = pak_parser_read_to_begin(itr, offset, pak_data->size());
    
		if (itr_000 == index) {
			offset = pak_parser_read_to_end(itr, offset, pak_data->size());
      return state->read_buffer;
    }
    state->read_buffer.clear();
  }
  return std::string();
}
worldmap_stage_file read_map_file_data(i32 index, size_t offset) {
  if (index < 0 or index > MAX_RESOURCE_FILES) {
    TraceLog(LOG_ERROR, "pak_parser::read_map_file_data()::File index is out of bound");
    return worldmap_stage_file();
  }
  const std::string& pak_data = state->map_pak_data;

  if (offset > pak_data.size()) {
    TraceLog(LOG_ERROR, "pak_parser::read_map_file_data()::offset is out of bound");
    return worldmap_stage_file();
  }
  map_pak_reading_order reading_order = MAP_PAK_READING_ORDER_UNDEFINED;
  worldmap_stage_file file = worldmap_stage_file();

  [[__maybe_unused__]] const std::string::const_iterator itr = pak_data.begin() + offset;
  i32 files_readed = 0;

  for (size_t itr_000 = offset; itr_000 < pak_data.size(); ++itr_000) {
    switch (reading_order) {
      default: {
        TraceLog(LOG_ERROR, "pak_parser::read_map_file_data()::Unsupported reading order stage");
        return worldmap_stage_file();
      }
    }
    reading_order = static_cast<map_pak_reading_order>((reading_order % (MAP_PAK_READING_ORDER_MAX)) + 1);
    if (reading_order == MAP_PAK_READING_ORDER_MAX) {
      if (files_readed == index) {
        return file;
        break;
      }
      files_readed++;
      file = worldmap_stage_file();
    }
    state->read_buffer.clear();
  }

  return worldmap_stage_file();
}

size_t pak_parser_read_to_begin(std::string::const_iterator iter, size_t offset, size_t delimiter) {
  size_t out_offset = 0;
  state->read_buffer.clear();
	
  for (size_t itr_000 = offset; offset + itr_000 + HEADER_SYMBOL_BEGIN_LENGTH < delimiter; ++itr_000) {

		std::string header_symbol = std::string(
			iter + itr_000, 
			iter + itr_000 + HEADER_SYMBOL_BEGIN_LENGTH
		);
    if (TextIsEqual(header_symbol.c_str(), HEADER_SYMBOL_BEGIN)) {
			break;
    }
    state->read_buffer.push_back(*(iter + itr_000));
    out_offset = itr_000;
  }
  return out_offset + HEADER_SYMBOL_BEGIN_LENGTH;
}

size_t pak_parser_read_to_end(std::string::const_iterator iter, size_t offset, size_t delimiter) {
  size_t out_offset = 0;
  state->read_buffer.clear();
	
  for (size_t itr_000 = offset; offset + itr_000 + HEADER_SYMBOL_BEGIN_LENGTH < delimiter; ++itr_000) {

		std::string header_symbol = std::string(
			iter + itr_000, 
			iter + itr_000 + HEADER_SYMBOL_END_LENGTH
		);
    if (TextIsEqual(header_symbol.c_str(), HEADER_SYMBOL_END)) {
			break;
    }
    state->read_buffer.push_back(*(iter + itr_000));
    out_offset = itr_000;
  }
  return out_offset + HEADER_SYMBOL_END_LENGTH;
}

std::string pak_id_to_file_name(pak_file_id id) {
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    TraceLog(LOG_ERROR, "pak_parser::pak_id_to_file_name()::File id is out of bound");
    return std::string("");
  }
  switch (id) {
    case PAK_FILE_ASSET1: return std::string("asset1.pak");
    
    case PAK_FILE_ASSET2: return std::string("asset2.pak");
    
    case PAK_FILE_MAP: return std::string("map.pak");
    
    default:{
      TraceLog(LOG_ERROR, "pak_parser::pak_id_to_file_name()::Unsupported pak id");
      return std::string("");
    }
  }
  
  TraceLog(LOG_ERROR, "pak_parser::pak_id_to_file_name()::Function ended unexpectedly");
  return std::string("");
}
std::string * pak_id_to_pak_data_pointer(pak_file_id id) {
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    TraceLog(LOG_ERROR, "pak_parser::pak_id_to_pak_data_pointer()::File id is out of bound");
    return nullptr;
  }
  switch (id) {
    case PAK_FILE_ASSET1: return __builtin_addressof(state->asset_pak_datas.at(0).pak_data);
    
    case PAK_FILE_ASSET2: return __builtin_addressof(state->asset_pak_datas.at(1).pak_data);
    
    case PAK_FILE_MAP: return __builtin_addressof(state->map_pak_data);
    
    default:{
      TraceLog(LOG_ERROR, "pak_parser::pak_id_to_pak_data_pointer()::Unsupported pak id");
      return nullptr;
    }
  }
  
  TraceLog(LOG_ERROR, "pak_parser::pak_id_to_pak_data_pointer()::Function ended unexpectedly");
  return nullptr;
}
file_buffer * pak_id_to_file_data_pointer(pak_file_id id, i32 index) {
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    TraceLog(LOG_ERROR, "pak_parser::pak_id_to_file_data_pointer()::File id is out of bound");
    return nullptr;
  }
  switch (id) {
    case PAK_FILE_ASSET1: return __builtin_addressof(state->asset_pak_datas.at(0).file_buffers.at(index-1));
    case PAK_FILE_ASSET2: return __builtin_addressof(state->asset_pak_datas.at(1).file_buffers.at(index-1));
    default:{
      TraceLog(LOG_ERROR, "pak_parser::pak_id_to_file_data_pointer()::Unsupported pak id");
      return nullptr;
    }
  }
  
  TraceLog(LOG_ERROR, "pak_parser::pak_id_to_file_data_pointer()::Function ended unexpectedly");
  return nullptr;
}
void assign_pak_data_by_id(pak_file_id id, std::string data) {
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    TraceLog(LOG_ERROR, "pak_parser::assign_pak_data_by_id()::File id is out of bound");
    return;
  }
  switch (id) {
    case PAK_FILE_ASSET1: {
      state->asset_pak_datas.at(0).pak_data.clear();
      state->asset_pak_datas.at(0).pak_data.assign(data);
      state->asset_pak_datas.at(0).is_initialized = true;
      return;
    }
    case PAK_FILE_ASSET2: {
      state->asset_pak_datas.at(1).pak_data.clear();
      state->asset_pak_datas.at(1).pak_data.assign(data);
      state->asset_pak_datas.at(1).is_initialized = true;
      return;
    }
    case PAK_FILE_MAP: {
      state->map_pak_data.clear();
      state->map_pak_data.assign(data);
      state->is_map_pak_data_initialized = true;
      return;
    }
    default:{
      TraceLog(LOG_ERROR, "pak_parser::pak_id_to_file_name()::Unsupported pak id");
      return;
    }
  }
  TraceLog(LOG_ERROR, "pak_parser::pak_id_to_file_name()::Function ended unexpectedly");
  return;
}

const file_buffer * fetch_asset_file_buffer(pak_file_id id, i32 index) {
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    TraceLog(LOG_WARNING, "pak_parser::fetch_asset_file_buffer()::Pak id is out of bound");
    return nullptr;
  }
  if (not state->asset_pak_datas.at(id-1).is_initialized) {
    const std::string path = pak_id_to_file_name(id);
    state->read_buffer.clear();
    if (not read_file(path.c_str())) {
      TraceLog(LOG_WARNING, "pak_parser::fetch_asset_file_buffer()::File read failed");
      return nullptr;
    }
    assign_pak_data_by_id(id, state->read_buffer);
  }
  file_buffer * const buffer = pak_id_to_file_data_pointer(id, index);
  if (not buffer or buffer == nullptr) {
    TraceLog(LOG_WARNING, "pak_parser::fetch_asset_file_buffer()::File pointer is invalid");
    return nullptr;
  }
	buffer->content = read_asset_file_data(id, index, 0);
  return buffer;
}
worldmap_stage_file fetch_map_file_data(pak_file_id id, i32 index) {
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    TraceLog(LOG_WARNING, "pak_parser::fetch_asset_file_buffer()::Pak id is out of bound");
    return worldmap_stage_file();
  }
  if (not state->asset_pak_datas.at(PAK_FILE_MAP-1).is_initialized) {
    const std::string path = pak_id_to_file_name(id);
    state->read_buffer.clear();
    if (read_file(path.c_str())) {
      TraceLog(LOG_WARNING, "pak_parser::fetch_map_file_data()::File read failed");
      return worldmap_stage_file();
    }
    assign_pak_data_by_id(id, state->read_buffer);
  }

  return read_map_file_data(index, 0);
}
