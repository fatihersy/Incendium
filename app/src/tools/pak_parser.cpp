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

  std::array<asset_pak_file, PAK_FILE_MAX> asset_pak_datas;
  std::string map_pak_data;
  bool is_map_pak_data_initialized;

  std::string read_buffer;
  pak_parser_system_state(void) {
    this->worldmap_location_file_datas.fill(worldmap_stage_file());

    this->asset_pak_datas.fill(asset_pak_file());
    this->map_pak_data = std::string("");
    this->is_map_pak_data_initialized = false;

    this->read_buffer = std::string("");
  }
} pak_parser_system_state;

static pak_parser_system_state * state = nullptr;
size_t read_asset_file_data(pak_file_id pak_id, size_t read_start_offset, size_t * out_file_start_offset);
worldmap_stage_file read_map_file_data(i32 index, size_t offset);

size_t pak_parser_read_to_begin(pak_file_id pak_id, size_t offset, size_t delimiter);
size_t pak_parser_read_to_end(pak_file_id pak_id, size_t offset, size_t delimiter);

std::string pak_id_to_file_name(pak_file_id id);
const asset_pak_file * pak_id_to_pak_file(pak_file_id id);
const file_buffer * pak_id_to_file_data_pointer(pak_file_id id, i32 index);
void assign_pak_data_by_id(pak_file_id id, std::string data);
void assign_file_data_by_id(pak_file_id id, i32 index, std::string* data, size_t file_offset_in_pak_data);

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

  state->asset_pak_datas.at(PAK_FILE_ASSET1) = asset_pak_file("asset1.pak", "asset1_files/",
    std::vector<file_buffer>({
      file_buffer(PAK_FILE_UNDEFINED, PAK_FILE_ASSET1_UNDEFINED, ""),
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
      file_buffer(PAK_FILE_UNDEFINED, PAK_FILE_ASSET1_MAX, ""),
    })
  );

  state->asset_pak_datas.at(PAK_FILE_ASSET2) = asset_pak_file("asset2.pak", "asset2_files/",
    std::vector<file_buffer>({
      file_buffer(PAK_FILE_UNDEFINED, PAK_FILE_ASSET2_UNDEFINED, ""),
      file_buffer(PAK_FILE_ASSET2, PAK_FILE_ASSET2_FADE_TRANSITION, "fs"),
      file_buffer(PAK_FILE_ASSET2, PAK_FILE_ASSET2_FONT_OUTLINE, "fs"),
      file_buffer(PAK_FILE_ASSET2, PAK_FILE_ASSET2_MAP_CHOICE_IMAGE, "fs"),
      file_buffer(PAK_FILE_ASSET2, PAK_FILE_ASSET2_POST_PROCESS, "fs"),
      file_buffer(PAK_FILE_ASSET2, PAK_FILE_ASSET2_PRG_BAR_MASK, "fs"),
      file_buffer(PAK_FILE_ASSET2, PAK_FILE_ASSET2_ATLAS, "png"),
      file_buffer(PAK_FILE_UNDEFINED, PAK_FILE_ASSET2_MAX, ""),
    })
  );

  return true;
}

bool parse_asset_pak(pak_file_id id) {
  if (not state or state == nullptr) {
    TraceLog(LOG_ERROR, "pak_parser::parse_pak()::State is invalid");
    return false;
  }
  switch (id) {
  	case PAK_FILE_ASSET1: {
  		if(not state->asset_pak_datas.at(id).is_initialized) {
  		  const std::string path = pak_id_to_file_name(id);
  		  state->read_buffer.clear();

  		  if (not read_file(path.c_str())) {
  		    TraceLog(LOG_ERROR, "pak_parser::parse_pak()::File read failed");
  		    return false;
  		  }
  		  assign_pak_data_by_id(id, state->read_buffer.data());
  		}

			size_t pak_file_offset = 0;
			size_t asset_file_start_offset = 0u;
  		for (i32 itr_000 = PAK_FILE_ASSET1_UNDEFINED+1; itr_000 < PAK_FILE_ASSET1_MAX; itr_000++) {
				if (pak_file_offset < state->asset_pak_datas.at(id).pak_data.size()) {
					pak_file_offset = read_asset_file_data(PAK_FILE_ASSET1, pak_file_offset, __builtin_addressof(asset_file_start_offset));
          assign_file_data_by_id(id, itr_000, __builtin_addressof(state->read_buffer), asset_file_start_offset);
				}
				else {
					return true;
				}
  		}
			return true;
		}
  	case PAK_FILE_ASSET2: {
  		if(not state->asset_pak_datas.at(id).is_initialized) {
  		  const std::string path = pak_id_to_file_name(id);
  		  state->read_buffer.clear();

  		  if (not read_file(path.c_str())) {
  		    TraceLog(LOG_ERROR, "pak_parser::parse_pak()::File read failed");
  		    return false;
  		  }
  		  assign_pak_data_by_id(id, state->read_buffer.data());
  		}
			size_t pak_file_offset = 0u;
			size_t asset_file_start_offset = 0u;
  		for (i32 itr_000 = PAK_FILE_ASSET2_UNDEFINED+1; itr_000 < PAK_FILE_ASSET2_MAX; itr_000++) {
				if (pak_file_offset < state->asset_pak_datas.at(id).pak_data.size()) {
					pak_file_offset = read_asset_file_data(PAK_FILE_ASSET2, pak_file_offset, __builtin_addressof(asset_file_start_offset));
					assign_file_data_by_id(id, itr_000, __builtin_addressof(state->read_buffer), asset_file_start_offset);
				}
				else {
					return true;
				}
  		}
			return true;
		}
  	default: {
  	  TraceLog(LOG_ERROR, "pak_parser::parse_asset_pak()::Unsupported Id");
  	  return false;
		}
  }
  TraceLog(LOG_ERROR, "pak_parser::parse_asset_pak()::Function ended unexpectedly");
  return false;
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
    case PAK_FILE_ASSET1: {
  		if (static_cast<size_t>(index) >= state->asset_pak_datas.at(id).file_buffers.size()) {
  		  TraceLog(LOG_ERROR, "pak_parser::get_asset_file_buffer()::Index is out of bound");
  		  return nullptr;
  		}
      file_buffer& file = state->asset_pak_datas.at(id).file_buffers.at(static_cast<size_t>(index));
      if (file.is_success) {
        return &file;
      }
      else {
        return fetch_asset_file_buffer(id, index);
      }
    }
    case PAK_FILE_ASSET2:{
  		if (static_cast<size_t>(index) >= state->asset_pak_datas.at(id).file_buffers.size()) {
  		  TraceLog(LOG_ERROR, "pak_parser::get_asset_file_buffer()::Index is out of bound");
  		  return nullptr;
  		}
      file_buffer& file = state->asset_pak_datas.at(id).file_buffers.at(static_cast<size_t>(index));
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
    return fetch_map_file_data(index);
  }
}

/**
 * @brief get_map_file_data() returns readed data unlike the fetch_file_data() reads when it's called
 */

size_t read_asset_file_data(pak_file_id pak_id, size_t read_start_offset, size_t * out_file_start_offset) {
  state->read_buffer.clear();
  const asset_pak_file * pak_file = pak_id_to_pak_file(pak_id);

  size_t accumulator = read_start_offset;

	accumulator = pak_parser_read_to_begin(pak_id, accumulator, pak_file->pak_data.size());

	std::string header_symbol = pak_file->pak_data.substr(accumulator, HEADER_SYMBOL_BEGIN_LENGTH);
	
  *out_file_start_offset = accumulator;
  return pak_parser_read_to_end(pak_id, accumulator, pak_file->pak_data.size());
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

size_t pak_parser_read_to_begin(pak_file_id pak_id, size_t offset, size_t delimiter) {
  size_t out_offset = offset;
  const asset_pak_file * pak_file = pak_id_to_pak_file(pak_id);
  state->read_buffer.clear();
	
  for (size_t itr_000 = out_offset; itr_000 + HEADER_SYMBOL_BEGIN_LENGTH < delimiter; ++itr_000) {

		std::string header_symbol = pak_file->pak_data.substr(itr_000, HEADER_SYMBOL_BEGIN_LENGTH);
    if (TextIsEqual(header_symbol.c_str(), HEADER_SYMBOL_BEGIN)) {
			break;
    }
    state->read_buffer.push_back(pak_file->pak_data.at(itr_000));
    out_offset = itr_000;
  }
  return out_offset + HEADER_SYMBOL_BEGIN_LENGTH;
}

size_t pak_parser_read_to_end(pak_file_id pak_id, size_t offset, size_t delimiter) {
  size_t out_offset = offset;
  const asset_pak_file * pak_file = pak_id_to_pak_file(pak_id);
  state->read_buffer.clear();
	
  for (size_t itr_000 = out_offset; itr_000 + HEADER_SYMBOL_BEGIN_LENGTH < delimiter; ++itr_000) {

		std::string header_symbol = pak_file->pak_data.substr(itr_000, HEADER_SYMBOL_END_LENGTH);
    if (TextIsEqual(header_symbol.c_str(), HEADER_SYMBOL_END)) {
      out_offset++;
			break;
    }
    state->read_buffer.push_back(pak_file->pak_data.at(itr_000));
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
const asset_pak_file * pak_id_to_pak_file(pak_file_id id) {
	if (not state or state == nullptr) {
    TraceLog(LOG_ERROR, "pak_parser::pak_id_to_pak_file()::State is invalid");
    return nullptr;
	}
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    TraceLog(LOG_ERROR, "pak_parser::pak_id_to_pak_file()::File id is out of bound");
    return nullptr;
  }
  switch (id) {
    case PAK_FILE_ASSET1: return __builtin_addressof(state->asset_pak_datas.at(id));
    case PAK_FILE_ASSET2: return __builtin_addressof(state->asset_pak_datas.at(id));
    default:{
      TraceLog(LOG_ERROR, "pak_parser::pak_id_to_pak_file()::Unsupported pak id");
      return nullptr;
    }
  }
  
  TraceLog(LOG_ERROR, "pak_parser::pak_id_to_pak_file()::Function ended unexpectedly");
  return nullptr;
}
const file_buffer * pak_id_to_file_data_pointer(pak_file_id id, i32 index) {
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    TraceLog(LOG_ERROR, "pak_parser::pak_id_to_file_data_pointer()::File id is out of bound");
    return nullptr;
  }
  switch (id) {
    case PAK_FILE_ASSET1: {
			if (static_cast<size_t>(index) >= state->asset_pak_datas.at(id).file_buffers.size()) {
    		TraceLog(LOG_ERROR, "pak_parser::assign_file_data_by_id()::Index is out of bound");
				return nullptr;
			}
			return __builtin_addressof(state->asset_pak_datas.at(id).file_buffers.at(index));
		}
    case PAK_FILE_ASSET2: {
			if (static_cast<size_t>(index) >= state->asset_pak_datas.at(id).file_buffers.size()) {
    		TraceLog(LOG_ERROR, "pak_parser::assign_file_data_by_id()::Index is out of bound");
				return nullptr;
			}
			return __builtin_addressof(state->asset_pak_datas.at(id).file_buffers.at(index));
		}
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
      state->asset_pak_datas.at(id).pak_data.clear();
      state->asset_pak_datas.at(id).pak_data.assign(data);
      state->asset_pak_datas.at(id).is_initialized = true;
      return;
    }
    case PAK_FILE_ASSET2: {
      state->asset_pak_datas.at(id).pak_data.clear();
      state->asset_pak_datas.at(id).pak_data.assign(data);
      state->asset_pak_datas.at(id).is_initialized = true;
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
void assign_file_data_by_id(pak_file_id id, i32 index, std::string* data, size_t file_offset_in_pak_data) {
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    TraceLog(LOG_ERROR, "pak_parser::assign_file_data_by_id()::File id is out of bound");
    return;
  }
  switch (id) {
    case PAK_FILE_ASSET1: {
			if (static_cast<size_t>(index) >= state->asset_pak_datas.at(id).file_buffers.size()) {
    		TraceLog(LOG_ERROR, "pak_parser::assign_file_data_by_id()::Index is out of bound");
				return;
			}
      state->asset_pak_datas.at(id).file_buffers.at(static_cast<size_t>(index)).content.clear();
      state->asset_pak_datas.at(id).file_buffers.at(static_cast<size_t>(index)).content.assign( (*data) );
			state->asset_pak_datas.at(id).file_buffers.at(static_cast<size_t>(index)).offset = file_offset_in_pak_data;
      return;
    }
    case PAK_FILE_ASSET2: {
			if (static_cast<size_t>(index) >= state->asset_pak_datas.at(id).file_buffers.size()) {
    		TraceLog(LOG_ERROR, "pak_parser::assign_file_data_by_id()::Index is out of bound");
				return;
			}
      state->asset_pak_datas.at(id).file_buffers.at(static_cast<size_t>(index)).content.clear();
      state->asset_pak_datas.at(id).file_buffers.at(static_cast<size_t>(index)).content.assign( (*data) );
			state->asset_pak_datas.at(id).file_buffers.at(static_cast<size_t>(index)).offset = file_offset_in_pak_data;
      return;
    }
    default:{
      TraceLog(LOG_ERROR, "pak_parser::assign_file_data_by_id()::Unsupported pak id");
      return;
    }
  }

  TraceLog(LOG_ERROR, "pak_parser::assign_file_data_by_id()::Function ended unexpectedly");
  return;
}

const file_buffer * fetch_asset_file_buffer(pak_file_id id, i32 index) {
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    TraceLog(LOG_WARNING, "pak_parser::fetch_asset_file_buffer()::Pak id is out of bound");
    return nullptr;
  }
  if (not state->asset_pak_datas.at(id).is_initialized) {
    const std::string path = pak_id_to_file_name(id);
    state->read_buffer.clear();
    if (not read_file(path.c_str())) {
      TraceLog(LOG_WARNING, "pak_parser::fetch_asset_file_buffer()::File read failed");
      return nullptr;
    }
    assign_pak_data_by_id(id, state->read_buffer);
  }
  const file_buffer * const buffer = pak_id_to_file_data_pointer(id, index);
  if (not buffer or buffer == nullptr) {
    TraceLog(LOG_WARNING, "pak_parser::fetch_asset_file_buffer()::File pointer is invalid");
    return nullptr;
  }
  size_t file_start_offset_in_pak= 0u;
	read_asset_file_data(id, 0, __builtin_addressof(file_start_offset_in_pak));
	assign_file_data_by_id(id, index, __builtin_addressof(state->read_buffer), file_start_offset_in_pak);
  return buffer;
}
worldmap_stage_file fetch_map_file_data(i32 index) {
  if (not state->asset_pak_datas.at(PAK_FILE_MAP).is_initialized) {
    const std::string path = pak_id_to_file_name(PAK_FILE_MAP);
    state->read_buffer.clear();
    if (read_file(path.c_str())) {
      TraceLog(LOG_WARNING, "pak_parser::fetch_map_file_data()::File read failed");
      return worldmap_stage_file();
    }
    assign_pak_data_by_id(PAK_FILE_MAP, state->read_buffer);
  }

  return read_map_file_data(index, 0);
}
