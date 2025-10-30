#include "pak_parser.h"
#include "raylib.h"

#include "core/fmemory.h"
#include "core/logger.h"

#define HEADER_SYMBOL_BEGIN "__BEGIN__"
#define HEADER_SYMBOL_BEGIN_LENGTH 9
#define HEADER_SYMBOL_END "__END__"
#define HEADER_SYMBOL_END_LENGTH 7

#define ASSET1_FILE_SIZE 146122975u
#define ASSET2_FILE_SIZE 3016472u // WARN: This is not exact number of asset2 pak file byte size 
#define MAP_FILE_SIZE 3624970u

typedef enum worldmap_data_type {
  WORLDMAP_DATA_UNDEFINED,
  WORLDMAP_DATA_COLLISION,
  WORLDMAP_DATA_LAYER1,
  WORLDMAP_DATA_LAYER2,
  WORLDMAP_DATA_LAYER3,
  WORLDMAP_DATA_LAYER4,
  WORLDMAP_DATA_PROP,
  WORLDMAP_DATA_MAX,
} worldmap_data_type;

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
} filename_offset_data;

typedef struct pak_parser_system_state {
  std::array<worldmap_stage_file, MAX_WORLDMAP_LOCATIONS> worldmap_location_file_datas;

  std::array<asset_pak_file, PAK_FILE_MAX> asset_pak_datas;
  std::string map_pak_data;
  bool is_map_pak_data_initialized;

  std::string read_buffer;
  worldmap_stage_file read_wsf_buffer;
  pak_parser_system_state(void) {
    this->worldmap_location_file_datas.fill(worldmap_stage_file());

    this->asset_pak_datas.fill(asset_pak_file());
    this->map_pak_data = std::string();
    this->is_map_pak_data_initialized = false;

    this->read_buffer = std::string();
    this->read_wsf_buffer = worldmap_stage_file();
  }
} pak_parser_system_state;

static pak_parser_system_state * state = nullptr;
size_t read_pak_file_data(pak_file_id pak_id, size_t read_start_offset, size_t * out_file_start_offset);

size_t pak_parser_read_to_header_begin(pak_file_id pak_id, size_t offset, size_t delimiter);
size_t pak_parser_read_to_header_end(pak_file_id pak_id, size_t offset, size_t delimiter);
size_t pak_parser_read_map_data(size_t offset, size_t *const out_pak_start_offset);

std::string pak_id_to_file_name(pak_file_id id);
const asset_pak_file * pak_id_to_pak_file(pak_file_id id);
const std::string * pak_id_to_pak_data_pointer(pak_file_id id);
const file_buffer * pak_id_to_file_data_pointer(pak_file_id id, i32 index);
void assign_pak_data_by_id(pak_file_id id);
void assign_file_data_by_id(pak_file_id id, i32 index, size_t file_offset_in_pak_data);

u64 get_file_size(pak_file_id id) {
  switch (id) {
    case PAK_FILE_ASSET1: return ASSET1_FILE_SIZE;
    case PAK_FILE_ASSET2: return ASSET2_FILE_SIZE;
    case PAK_FILE_MAP: return MAP_FILE_SIZE;
    default: return 0u;
  }
}

i32 read_file(const char * path, u64 max_size) {
  if (not FileExists(path)) {
    IERROR("main::write_map_pak()::file '%s' doesn't exist", path);
    return 0;
  }
  state->read_buffer.clear();
  i32 loaded_data = 1;
  u8* data = LoadFileData(path, &loaded_data);
  if (loaded_data <= 1 or static_cast<u64>(loaded_data) > max_size) {
    IERROR("pak_parser::read_file()::file '%s' doesn't exist or exceeds the maximum size", path);
    return 0;
  }
  state->read_buffer.assign(data, data + loaded_data);

  UnloadFileData(data);
	return loaded_data;
}

bool pak_parser_system_initialize(void) {
  if (state and state != nullptr) {
    IERROR("pak_parser::pak_parser_system_initialize()::Called twice");
    return true;
  }
  state = (pak_parser_system_state*)allocate_memory_linear(sizeof(pak_parser_system_state), true);
  if (not state or state == nullptr) {
    IERROR("pak_parser::pak_parser_system_initialize()::State allocation failed");
    return false;
  }
  *state = pak_parser_system_state();

  state->asset_pak_datas.at(PAK_FILE_ASSET1) = asset_pak_file("asset1.pak", "asset1_files/",
    std::vector<file_buffer>({
      file_buffer(PAK_FILE_UNDEFINED, PAK_FILE_ASSET1_UNDEFINED, ""),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_THUMBNAIL,               ".png"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_WORLDMAP_IMAGE,          ".png"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_BLACK_BACKGROUND_IMAGE1, ".png"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_BLACK_BACKGROUND_IMAGE2, ".png"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SPRITESHEET_ZAP,         ".png"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_FONT_MOOD,               ".ttf"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_FONT_MIOSEVKA_ITALIC,    ".ttf"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_FONT_MIOSEVKA_LIGHT,     ".ttf"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_FONT_MIOSEVKA_REGULAR,   ".ttf"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_FONT_MIOSEVKA_BOLD,      ".ttf"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_COIN_PICKUP,             ".mp3"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_HEALTH_PICKUP,           ".mp3"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SOUND_LEVEL_UP,          ".mp3"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SOUND_BTN_CLICK_1,       ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SOUND_BTN_CLICK_2,       ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SOUND_BTN_CLICK_3,       ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SOUND_DENY1,             ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SOUND_DENY2,             ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_EXP_PICKUP,              ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SOUND_GAME_OVER,         ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SOUND_ZAP_1,             ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SOUND_ZAP_2,             ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SOUND_ZAP_3,             ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SOUND_ZAP_4,             ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SOUND_ZOMBIE_DIE_1,      ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SOUND_ZOMBIE_DIE_2,      ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SOUND_ZOMBIE_DIE_3,      ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_THEME_MAINMENU_1,        ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_THEME_MAINMENU_2,        ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_THEME_MAINMENU_3,        ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_THEME_MAP_SELECTION_1,   ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_THEME_MAP_SELECTION_2,   ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_THEME_INGAME_PLAY_1,     ".ogg"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_THEME_INGAME_PLAY_2,     ".ogg"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_THEME_INGAME_PLAY_3,     ".ogg"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_THEME_INGAME_PLAY_4,     ".ogg"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SPIN_RESULT_STAR_105_105,".png"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SPIN_SFX,                ".wav"),
      file_buffer(PAK_FILE_ASSET1,    PAK_FILE_ASSET1_SPIN_RESULT,             ".wav"),
      file_buffer(PAK_FILE_UNDEFINED, PAK_FILE_ASSET1_MAX, ""),
    })
  );
  state->asset_pak_datas.at(PAK_FILE_ASSET2) = asset_pak_file("asset2.pak", "asset2_files/",
    std::vector<file_buffer>({
      file_buffer(PAK_FILE_UNDEFINED, PAK_FILE_ASSET2_UNDEFINED, ""),
      file_buffer(PAK_FILE_ASSET2,    PAK_FILE_ASSET2_LOC_FILE_ENGLISH,"._loc_data"),
      file_buffer(PAK_FILE_ASSET2,    PAK_FILE_ASSET2_LOC_FILE_TURKISH,"._loc_data"),
      file_buffer(PAK_FILE_ASSET2,    PAK_FILE_ASSET2_ATLAS,           ".png"),
      file_buffer(PAK_FILE_ASSET2,    PAK_FILE_ASSET2_FADE_TRANSITION, ".fs"),
      file_buffer(PAK_FILE_ASSET2,    PAK_FILE_ASSET2_FONT_OUTLINE,    ".fs"),
      file_buffer(PAK_FILE_ASSET2,    PAK_FILE_ASSET2_MAP_CHOICE_IMAGE,".fs"),
      file_buffer(PAK_FILE_ASSET2,    PAK_FILE_ASSET2_POST_PROCESS,    ".fs"),
      file_buffer(PAK_FILE_ASSET2,    PAK_FILE_ASSET2_PRG_BAR_MASK,    ".fs"),
      file_buffer(PAK_FILE_ASSET2,    PAK_FILE_ASSET2_SDR_SPAWN,       ".fs"),
      file_buffer(PAK_FILE_ASSET2,    PAK_FILE_ASSET2_SDF_TEXT,        ".fs"),
      file_buffer(PAK_FILE_ASSET2,    PAK_FILE_ASSET2_SDR_SPIN,        ".fs"),
      file_buffer(PAK_FILE_UNDEFINED, PAK_FILE_ASSET2_MAX, ""),
    })
  );
  return true;
}

bool parse_asset_pak(pak_file_id id) {
  if (not state or state == nullptr) {
    IERROR("pak_parser::parse_asset_pak()::State is invalid");
    return false;
  }
  switch (id) {
  	case PAK_FILE_ASSET1: {
  		if(not state->asset_pak_datas.at(id).is_initialized) {
  		  const std::string path = pak_id_to_file_name(id);
  		  state->read_buffer.clear();

  		  if (not read_file(path.c_str(), get_file_size(id))) {
  		    IERROR("pak_parser::parse_asset_pak()::File read failed");
  		    return false;
  		  }
  		  assign_pak_data_by_id(id);
  		}

			size_t pak_file_offset = 0u;
			size_t asset_file_start_offset = 0u;
  		for (i32 itr_000 = PAK_FILE_ASSET1_UNDEFINED+1; itr_000 < PAK_FILE_ASSET1_MAX; itr_000++) {
				if (pak_file_offset < state->asset_pak_datas.at(id).pak_data.size()) {
					pak_file_offset = read_pak_file_data(PAK_FILE_ASSET1, pak_file_offset, __builtin_addressof(asset_file_start_offset));
          assign_file_data_by_id(id, itr_000, asset_file_start_offset);
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

  		  if (not read_file(path.c_str(), get_file_size(id))) {
  		    IERROR("pak_parser::parse_asset_pak()::File read failed");
  		    return false;
  		  }
        //std::string read_buffer_end = state->read_buffer.substr(state->read_buffer.size() - 16u, 16u);
  		  assign_pak_data_by_id(id);
  		}
			size_t pak_file_offset = 0u;
			size_t asset_file_start_offset = 0u;
  		for (i32 itr_000 = PAK_FILE_ASSET2_UNDEFINED+1; itr_000 < PAK_FILE_ASSET2_MAX; itr_000++) {
				if (pak_file_offset < state->asset_pak_datas.at(id).pak_data.size()) {
					pak_file_offset = read_pak_file_data(PAK_FILE_ASSET2, pak_file_offset, __builtin_addressof(asset_file_start_offset));
					assign_file_data_by_id(id, itr_000, asset_file_start_offset);
				}
				else {
					return true;
				}
  		}
			return true;
		}
  	default: {
  	  IWARN("pak_parser::parse_asset_pak()::Unsupported Id");
  	  return false;
		}
  }
  IERROR("pak_parser::parse_asset_pak()::Function ended unexpectedly");
  return false;
}
bool parse_map_pak(void) {
  if (not state or state == nullptr) {
    IERROR("pak_parser::parse_map_pak()::State is invalid");
    return false;
  }
  if(state->map_pak_data.empty()) {
    const std::string path = pak_id_to_file_name(PAK_FILE_MAP);
    state->read_buffer.clear();
    if (not read_file(path.c_str(), get_file_size(PAK_FILE_MAP))) {
      IERROR("pak_parser::parse_map_pak()::Pak file:'%s' read failed", path.c_str());
      return false;
    }
    assign_pak_data_by_id(PAK_FILE_MAP);
  }
	size_t pak_file_offset = 0u;
	size_t pak_file_start_offset = 0u;
  for (i32 itr_000 = 0; itr_000 < MAX_WORLDMAP_LOCATIONS; itr_000++) {
		if (pak_file_offset < state->map_pak_data.size()) {
      pak_file_offset = pak_parser_read_map_data(pak_file_offset, __builtin_addressof(pak_file_start_offset));
      assign_file_data_by_id(PAK_FILE_MAP, itr_000, pak_file_start_offset);
      state->worldmap_location_file_datas.at(itr_000).is_success = true;
		}
		else {
			return true;
		}
  }
	return true;
}

size_t read_pak_file_data(pak_file_id pak_id, size_t read_start_offset, size_t * out_file_start_offset) {
  state->read_buffer.clear();
  const std::string * pak_data = pak_id_to_pak_data_pointer(pak_id);
  if (not pak_data or pak_data == nullptr) {
    IERROR("pak_parser::read_pak_file_data()::Pak data is invalid");
    return 0u;
  }
  size_t accumulator = read_start_offset;

	accumulator = pak_parser_read_to_header_begin(pak_id, accumulator, pak_data->size());

  //std::string header_begin = std::string();
  //if (accumulator + HEADER_SYMBOL_BEGIN_LENGTH <= pak_file->pak_data.size()) {
  //  header_begin = pak_file->pak_data.substr(accumulator, HEADER_SYMBOL_BEGIN_LENGTH);
  //}
  //else {
  //  header_begin = pak_file->pak_data.substr(accumulator, pak_file->pak_data.size() - accumulator);
  //}

  *out_file_start_offset = accumulator;
  size_t out_size = pak_parser_read_to_header_end(pak_id, accumulator, pak_data->size());

  //std::string header_end = state->read_buffer.substr(state->read_buffer.size() - 16u, 16u);

  return out_size;
}

size_t pak_parser_read_to_header_begin(pak_file_id pak_id, size_t offset, size_t delimiter) {
  state->read_buffer.clear();
  const std::string * pak_data = pak_id_to_pak_data_pointer(pak_id);
  if (not pak_data or pak_data == nullptr) {
    IERROR("pak_parser::read_pak_file_data()::Pak data is invalid");
    return 0u;
  }
  size_t out_offset = offset;
	
  for (size_t itr_000 = out_offset; itr_000 + HEADER_SYMBOL_BEGIN_LENGTH < delimiter; ++itr_000) {

		std::string header_symbol = pak_data->substr(itr_000, HEADER_SYMBOL_BEGIN_LENGTH);
    if (TextIsEqual(header_symbol.c_str(), HEADER_SYMBOL_BEGIN)) {
			break;
    }
    state->read_buffer.push_back(pak_data->at(itr_000));
    out_offset = itr_000;
  }
  if (out_offset + HEADER_SYMBOL_BEGIN_LENGTH > delimiter) {
    return delimiter;
  }
  else {
    return out_offset + HEADER_SYMBOL_BEGIN_LENGTH;
  }
}
size_t pak_parser_read_to_header_end(pak_file_id pak_id, size_t offset, size_t delimiter) {
  state->read_buffer.clear();
  const std::string * pak_data = pak_id_to_pak_data_pointer(pak_id);
  if (not pak_data or pak_data == nullptr) {
    IERROR("pak_parser::read_pak_file_data()::Pak data is invalid");
    return 0u;
  }
  size_t out_offset = offset;
	
  for (size_t itr_000 = out_offset; itr_000 + HEADER_SYMBOL_END_LENGTH < delimiter; ++itr_000) {
    out_offset = itr_000;

		std::string header_symbol = pak_data->substr(itr_000, HEADER_SYMBOL_END_LENGTH);
    if (TextIsEqual(header_symbol.c_str(), HEADER_SYMBOL_END)) {
			break;
    }
    state->read_buffer.push_back(pak_data->at(itr_000));
  }

  if (out_offset + HEADER_SYMBOL_END_LENGTH > delimiter) {
    return delimiter;
  }
  else {
    return out_offset + HEADER_SYMBOL_END_LENGTH;
  }
}
size_t pak_parser_read_map_data(size_t offset, size_t *const out_pak_start_offset) {
  state->read_buffer.clear();
  size_t out_offset = offset;
  size_t file_start = 0u;

  out_offset = read_pak_file_data(PAK_FILE_MAP, out_offset, __builtin_addressof(file_start));
  state->read_wsf_buffer.file_collision = state->read_buffer;
  *out_pak_start_offset = file_start;

  out_offset = read_pak_file_data(PAK_FILE_MAP, out_offset, __builtin_addressof(file_start));
  state->read_wsf_buffer.layer_data.at(0) = state->read_buffer;
  out_offset = read_pak_file_data(PAK_FILE_MAP, out_offset, __builtin_addressof(file_start));
  state->read_wsf_buffer.layer_data.at(1) = state->read_buffer;
  out_offset = read_pak_file_data(PAK_FILE_MAP, out_offset, __builtin_addressof(file_start));
  state->read_wsf_buffer.layer_data.at(2) = state->read_buffer;
  out_offset = read_pak_file_data(PAK_FILE_MAP, out_offset, __builtin_addressof(file_start));
  state->read_wsf_buffer.layer_data.at(3) = state->read_buffer;
  out_offset = read_pak_file_data(PAK_FILE_MAP, out_offset, __builtin_addressof(file_start));
  state->read_wsf_buffer.layer_data.at(4) = state->read_buffer;
  
  out_offset = read_pak_file_data(PAK_FILE_MAP, out_offset, __builtin_addressof(file_start));
  state->read_wsf_buffer.file_prop = state->read_buffer;
  return out_offset;
}

std::string pak_id_to_file_name(pak_file_id id) {
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    IWARN("pak_parser::pak_id_to_file_name()::File id is out of bound");
    return std::string();
  }
  switch (id) {
    case PAK_FILE_ASSET1: return std::string("asset1.pak");
    case PAK_FILE_ASSET2: return std::string("asset2.pak");
    case PAK_FILE_MAP: return std::string("map.pak");
    default:{
      IWARN("pak_parser::pak_id_to_file_name()::Unsupported pak id");
      return std::string();
    }
  }
  IERROR("pak_parser::pak_id_to_file_name()::Function ended unexpectedly");
  return std::string();
}
const asset_pak_file * pak_id_to_pak_file(pak_file_id id) {
	if (not state or state == nullptr) {
    IERROR("pak_parser::pak_id_to_pak_file()::State is invalid");
    return nullptr;
	}
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    IWARN("pak_parser::pak_id_to_pak_file()::File id is out of bound");
    return nullptr;
  }
  switch (id) {
    case PAK_FILE_ASSET1: return __builtin_addressof(state->asset_pak_datas.at(id));
    case PAK_FILE_ASSET2: return __builtin_addressof(state->asset_pak_datas.at(id));
    default:{
      IWARN("pak_parser::pak_id_to_pak_file()::Unsupported pak id");
      return nullptr;
    }
  }
  IERROR("pak_parser::pak_id_to_pak_file()::Function ended unexpectedly");
  return nullptr;
}
const std::string * pak_id_to_pak_data_pointer(pak_file_id id) {
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    IWARN("pak_parser::pak_id_to_pak_data_pointer()::File id is out of bound");
    return nullptr;
  }
  switch (id) {
    case PAK_FILE_ASSET1: {
			return __builtin_addressof(state->asset_pak_datas.at(id).pak_data);
		}
    case PAK_FILE_ASSET2: {
			return __builtin_addressof(state->asset_pak_datas.at(id).pak_data);
		}
    case PAK_FILE_MAP: {
			return __builtin_addressof(state->map_pak_data);
		}
    default:{
      IWARN("pak_parser::pak_id_to_pak_data_pointer()::Unsupported pak id");
      return nullptr;
    }
  }
  IERROR("pak_parser::pak_id_to_pak_data_pointer()::Function ended unexpectedly");
  return nullptr;
}
const file_buffer * pak_id_to_file_data_pointer(pak_file_id id, i32 index) {
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    IERROR("pak_parser::pak_id_to_file_data_pointer()::File id is out of bound");
    return nullptr;
  }
  switch (id) {
    case PAK_FILE_ASSET1: {
			if (static_cast<size_t>(index) >= state->asset_pak_datas.at(id).file_buffers.size()) {
    		IWARN("pak_parser::pak_id_to_file_data_pointer()::Index is out of bound");
				return nullptr;
			}
			return __builtin_addressof(state->asset_pak_datas.at(id).file_buffers.at(index));
		}
    case PAK_FILE_ASSET2: {
			if (static_cast<size_t>(index) >= state->asset_pak_datas.at(id).file_buffers.size()) {
    		IWARN("pak_parser::pak_id_to_file_data_pointer()::Index is out of bound");
				return nullptr;
			}
			return __builtin_addressof(state->asset_pak_datas.at(id).file_buffers.at(index));
		}
    default:{
      IWARN("pak_parser::pak_id_to_file_data_pointer()::Unsupported pak id");
      return nullptr;
    }
  }
  IERROR("pak_parser::pak_id_to_file_data_pointer()::Function ended unexpectedly");
  return nullptr;
}
void assign_pak_data_by_id(pak_file_id id) {
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    IWARN("pak_parser::assign_pak_data_by_id()::File id is out of bound");
    return;
  }
  switch (id) {
    case PAK_FILE_ASSET1: {
      state->asset_pak_datas.at(id).pak_data.clear();
      state->asset_pak_datas.at(id).pak_data.assign(state->read_buffer);
      state->asset_pak_datas.at(id).is_initialized = true;
      return;
    }
    case PAK_FILE_ASSET2: {
      state->asset_pak_datas.at(id).pak_data.clear();
      state->asset_pak_datas.at(id).pak_data.assign(state->read_buffer);
      state->asset_pak_datas.at(id).is_initialized = true;
      return;
    }
    case PAK_FILE_MAP: {
      state->map_pak_data.clear();
      state->map_pak_data.assign(state->read_buffer);
      state->is_map_pak_data_initialized = true;
      return;
    }
    default:{
      IWARN("pak_parser::pak_id_to_file_name()::Unsupported pak id");
      return;
    }
  }
  IERROR("pak_parser::pak_id_to_file_name()::Function ended unexpectedly");
  return;
}
void assign_file_data_by_id(pak_file_id id, i32 index, size_t file_offset_in_pak_data) {
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    IWARN("pak_parser::assign_file_data_by_id()::File id is out of bound");
    return;
  }
  switch (id) {
    case PAK_FILE_ASSET1: {
			if (static_cast<size_t>(index) >= state->asset_pak_datas.at(id).file_buffers.size()) {
    		IERROR("pak_parser::assign_file_data_by_id()::Index is out of bound");
				return;
			}
      //std::string header_begin = state->read_buffer.substr(0, HEADER_SYMBOL_BEGIN_LENGTH);
      //std::string header_end = state->read_buffer.substr(state->read_buffer.size() - 16u, 16u);

      state->asset_pak_datas.at(id).file_buffers.at(static_cast<size_t>(index)).content.clear();
      state->asset_pak_datas.at(id).file_buffers.at(static_cast<size_t>(index)).content.assign(state->read_buffer);
			state->asset_pak_datas.at(id).file_buffers.at(static_cast<size_t>(index)).offset = file_offset_in_pak_data;
			state->asset_pak_datas.at(id).file_buffers.at(static_cast<size_t>(index)).is_success = true;
      return;
    }
    case PAK_FILE_ASSET2: {
			if (static_cast<size_t>(index) >= state->asset_pak_datas.at(id).file_buffers.size()) {
    		IWARN("pak_parser::assign_file_data_by_id()::Index is out of bound");
				return;
			}
      state->asset_pak_datas.at(id).file_buffers.at(static_cast<size_t>(index)).content.clear();
      state->asset_pak_datas.at(id).file_buffers.at(static_cast<size_t>(index)).content.assign(state->read_buffer);
			state->asset_pak_datas.at(id).file_buffers.at(static_cast<size_t>(index)).offset = file_offset_in_pak_data;
			state->asset_pak_datas.at(id).file_buffers.at(static_cast<size_t>(index)).is_success = true;
      return;
    }
    case PAK_FILE_MAP: {
			if (static_cast<size_t>(index) >= state->worldmap_location_file_datas.size()) {
    		IWARN("pak_parser::assign_file_data_by_id()::Index is out of bound");
				return;
			}
      state->worldmap_location_file_datas.at(index) = state->read_wsf_buffer;
      return;
    }
    default:{
      IWARN("pak_parser::assign_file_data_by_id()::Unsupported pak id");
      return;
    }
  }
  IERROR("pak_parser::assign_file_data_by_id()::Function ended unexpectedly");
  return;
}

const file_buffer * get_asset_file_buffer(pak_file_id id, i32 index) {
  if (not state or state == nullptr) {
    IERROR("pak_parser::get_asset_file_buffer()::Pak parser system didn't initialized");
    return nullptr;
  }
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    IWARN("pak_parser::get_asset_file_buffer()::File id is out of bound");
    return nullptr;
  }
  switch (id) {
    case PAK_FILE_ASSET1: {
  		if (static_cast<size_t>(index) >= state->asset_pak_datas.at(id).file_buffers.size()) {
  		  IWARN("pak_parser::get_asset_file_buffer()::Index is out of bound");
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
  		  IWARN("pak_parser::get_asset_file_buffer()::Index is out of bound");
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
      IWARN("pak_parser::get_asset_file_buffer()::Unsupported pak id");
      return nullptr;
    }
  }
  IERROR("pak_parser::get_asset_file_buffer()::Function ended unexpectedly");
  return nullptr;
}
const file_buffer * fetch_asset_file_buffer(pak_file_id id, i32 index) {
  if (id >= PAK_FILE_MAX or id <= PAK_FILE_UNDEFINED) {
    IWARN("pak_parser::fetch_asset_file_buffer()::Pak id is out of bound");
    return nullptr;
  }
  if (not state->asset_pak_datas.at(id).is_initialized) {
    const std::string path = pak_id_to_file_name(id);
    state->read_buffer.clear();
    if (not read_file(path.c_str(), get_file_size(id))) {
      IERROR("pak_parser::fetch_asset_file_buffer()::File read failed");
      return nullptr;
    }
    assign_pak_data_by_id(id);
  }
  if (id == PAK_FILE_ASSET1 and (index <= PAK_FILE_ASSET1_UNDEFINED or index >= PAK_FILE_ASSET1_MAX)) {
    return nullptr;
  }
  else if (id == PAK_FILE_ASSET2 and (index <= PAK_FILE_ASSET2_UNDEFINED or index >= PAK_FILE_ASSET2_MAX)) {
    return nullptr;
  }
  const file_buffer * const buffer = pak_id_to_file_data_pointer(id, index);
  if (not buffer or buffer == nullptr) {
    IERROR("pak_parser::fetch_asset_file_buffer()::File pointer is invalid");
    return nullptr;
  }
  size_t file_start_offset_in_pak= 0u;
  for (i32 itr_000 = 0; itr_000 < index; ++itr_000) {
	  read_pak_file_data(id, file_start_offset_in_pak, __builtin_addressof(file_start_offset_in_pak));
  }
	assign_file_data_by_id(id, index, file_start_offset_in_pak);
  return buffer;
}

const worldmap_stage_file * get_map_file_buffer(i32 index) {
  if (not state or state == nullptr) {
    IERROR("pak_parser::get_asset_file_buffer()::Pak parser system didn't initialized");
    return nullptr;
  }
  if (index >= MAX_WORLDMAP_LOCATIONS or index < 0) {
    IWARN("pak_parser::get_asset_file_buffer()::File id is out of bound");
    return nullptr;
  }
  worldmap_stage_file& file = state->worldmap_location_file_datas.at(index);
  return file.is_success ? &file : fetch_map_file_buffer(index);
}
const worldmap_stage_file * fetch_map_file_buffer(i32 index) {
  if (state->map_pak_data.empty()) {
    const std::string path = pak_id_to_file_name(PAK_FILE_MAP);
    state->read_buffer.clear();
    if (not read_file(path.c_str(), get_file_size(PAK_FILE_MAP))) {
      IERROR("pak_parser::fetch_asset_file_buffer()::File read failed");
      return nullptr;
    }
    assign_pak_data_by_id(PAK_FILE_MAP);
  }
  if (index < 0 or index >= MAX_WORLDMAP_LOCATIONS) {
    return nullptr;
  }
  size_t file_start_offset_in_pak= 0u;
  for (i32 itr_000 = 0; itr_000 < index; ++itr_000) {
	  pak_parser_read_map_data(file_start_offset_in_pak, __builtin_addressof(file_start_offset_in_pak));
    assign_file_data_by_id(PAK_FILE_MAP, index, file_start_offset_in_pak);
  }

  return __builtin_addressof(state->worldmap_location_file_datas.at(index));;
}
