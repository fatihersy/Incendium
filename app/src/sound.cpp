#include "sound.h"
#include "raylib.h"

#include "core/fmemory.h"
#include "core/event.h"
#include "core/logger.h"

#if USE_PAK_FORMAT 
#include "tools/pak_parser.h"
#endif

#include "game/game_types.h"

// TODO: Destroy sound system

typedef struct sound_system_state {
  std::array<sound_data, SOUND_ID_MAX> sounds;
  std::array<music_data, MUSIC_ID_MAX> musics;

  #if not USE_PAK_FORMAT
  std::array<file_buffer, SOUND_ID_MAX> sound_datas; 
  std::array<file_buffer, MUSIC_ID_MAX> music_datas; 
  #endif

  sound_system_state(void) {
    this->sounds.fill(sound_data());
    this->musics.fill(music_data());
  }
}sound_system_state;

static sound_system_state * state = nullptr;

void load_sound_pak(pak_file_id pak_id, i32 file_id, sound_id id);
void load_music_pak(pak_file_id pak_id, i32 file_id, music_id id);
void load_sound_disk(const char * path, sound_id id);
void load_music_disk(const char * path, music_id id);

bool sound_system_on_event(i32 code, event_context context);

/**
 * @brief Depends on pak_parser, init after that
 */
bool sound_system_initialize(void) {
  if (state and state != nullptr) {
    return true;
  }
  state = (sound_system_state*)allocate_memory_linear(sizeof(sound_system_state), true);
  if (not state or state == nullptr) {
    IERROR("sound::sound_system_initialize()::State allocation failed");
    return false;
  }
  *state = sound_system_state();

  InitAudioDevice();
  
  #if USE_PAK_FORMAT
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_BTN_CLICK_1, SOUND_ID_BUTTON_ON_CLICK);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_DENY, SOUND_ID_DENY);
  //load_sound(PAK_FILE_ASSET1, "fire_hit.wav", SOUND_ID_FIRE_ON_HIT);
  load_music_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_MUSIC_MAIN_MENU_THEME, MUSIC_ID_MAIN_MENU_THEME);
  load_music_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_MUSIC_NIGHT_THEME, MUSIC_ID_NIGHT_THEME2);
  load_music_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_MUSIC_TRACK_5, MUSIC_ID_TRACK5);
  #else
  load_sound_disk("button_click1.wav", SOUND_ID_BUTTON_ON_CLICK);
  load_sound_disk("deny.wav", SOUND_ID_DENY);
  //load_sound("fire_hit.wav", SOUND_ID_FIRE_ON_HIT);
  load_music_disk("main_menu_theme.wav", MUSIC_ID_MAIN_MENU_THEME);
  load_music_disk("night_theme_2.wav", MUSIC_ID_NIGHT_THEME2);
  load_music_disk("Track_#5.wav", MUSIC_ID_TRACK5);
  #endif

  event_register(EVENT_CODE_PLAY_BUTTON_ON_CLICK, sound_system_on_event);
  event_register(EVENT_CODE_PLAY_FIRE_HIT, sound_system_on_event);
  event_register(EVENT_CODE_PLAY_MAIN_MENU_THEME, sound_system_on_event);
  event_register(EVENT_CODE_PLAY_NIGHT_THEME2, sound_system_on_event);
  event_register(EVENT_CODE_PLAY_TRACK5, sound_system_on_event);
  event_register(EVENT_CODE_PLAY_SOUND, sound_system_on_event);
  event_register(EVENT_CODE_PLAY_MUSIC, sound_system_on_event);
  event_register(EVENT_CODE_RESET_SOUND, sound_system_on_event);
  event_register(EVENT_CODE_RESET_MUSIC, sound_system_on_event);

  return true;
}

void update_sound_system(void) {
  if (not state or state == nullptr) {
    IERROR("sound::update_sound_system()::State is not valid");
    return;
  }

  for (size_t itr_000 = 0; itr_000 < MUSIC_ID_MAX; ++itr_000) {
    UpdateMusicStream(state->musics.at(itr_000).handle);
  }
}

void load_sound_pak([[__maybe_unused__]] pak_file_id pak_file, [[__maybe_unused__]] i32 file_id, [[__maybe_unused__]] sound_id id) {
  #if USE_PAK_FORMAT
  const file_buffer * file = nullptr;
  Wave wav;
  sound_data data;

  file = get_asset_file_buffer(pak_file, file_id);
  if (not file or file == nullptr) {
    IWARN("sound::load_sound_pak()::File %d:%d is invalid", pak_file, file_id);
    return;
  }
  wav = LoadWaveFromMemory(file->file_extension.c_str(), reinterpret_cast<const u8*>(file->content.data()), file->content.size());
  if (not file->is_success) {
    IWARN("sound::load_sound_pak()::File id %d:%d cannot load successfully", pak_file, file_id);
    return;
  }
  
  data.wav = wav;
  data.file = file;
  data.id = id;
  data.handle = LoadSoundFromWave(wav);
  state->sounds.at(id) = data;
  #endif
}
void load_sound_disk([[__maybe_unused__]] const char * path, [[__maybe_unused__]] sound_id id) {
  #if not USE_PAK_FORMAT
  const file_buffer * file = __builtin_addressof(state->sound_datas.at(id));
  Wave wav;
  sound_data data;

  wav = LoadWave(TextFormat("%s%s", RESOURCE_PATH, path));

  data.wav = wav;
  data.file = file;
  data.id = id;
  data.handle = LoadSoundFromWave(wav);
  state->sounds.at(id) = data;
  #endif
}

void load_music_pak([[__maybe_unused__]] pak_file_id pak_file, [[__maybe_unused__]] i32 file_id, [[__maybe_unused__]] music_id id) {
  #if USE_PAK_FORMAT
  const file_buffer * file = nullptr;
  Music music;

  file = get_asset_file_buffer(pak_file, file_id);
  if (not file or file == nullptr) {
    IWARN("sound::load_music_pak()::File %d:%d is invalid", pak_file, file_id);
    return;
  }
  music = LoadMusicStreamFromMemory(file->file_extension.c_str(), reinterpret_cast<const u8*>(file->content.data()), file->content.size());
  if (not file->is_success) {
    IWARN("sound::load_music_pak()::File id %d cannot load successfully", file->file_id);
    return;
  }
  
  music_data data = music_data();
  data.file = file;
  data.id = id;
  data.handle = music;
  state->musics.at(id) = data;
  #endif
}

void load_music_disk([[__maybe_unused__]] const char * path, [[__maybe_unused__]] music_id id) {
  #if not USE_PAK_FORMAT
  const file_buffer * file = __builtin_addressof(state->sound_datas.at(id));
  Music music;

  music = LoadMusicStream(TextFormat("%s%s", RESOURCE_PATH, path));
  
  music_data data = music_data();
  data.file = file;
  data.id = id;
  data.handle = music;
  state->musics.at(id) = data;
  #endif
}

void play_sound(sound_id id) {
  if (not state or state == nullptr) {
    IERROR("sound::play_sound()::State is not valid");
    return;
  }
  if (id >= SOUND_ID_MAX or id <= SOUND_ID_UNSPECIFIED) {
    IWARN("sound::play_sound()::Sound id is out of bound");
    return;
  }

  PlaySound(state->sounds.at(id).handle);
}

void play_music(music_id id) {
  if (not state or state == nullptr) {
    IERROR("sound::play_music()::State is not valid");
    return;
  }
  if (id >= MUSIC_ID_MAX or id <= MUSIC_ID_UNSPECIFIED) {
    IWARN("sound::play_music()::Music id is out of bound");
    return;
  }

  PlayMusicStream(state->musics.at(id).handle);
}
void reset_music(music_id id) {
  if (not state or state == nullptr) {
    IERROR("sound::reset_music()::State is not valid");
    return;
  }
  if (id >= MUSIC_ID_MAX or id <= MUSIC_ID_UNSPECIFIED) {
    IWARN("sound::reset_music()::Music id is out of bound");
    return;
  }
  state->musics.at(id).play_once = false;
  state->musics.at(id).played = false;
  StopMusicStream(state->musics.at(id).handle);
}
void reset_sound(sound_id id) {
  if (not state or state == nullptr) {
    IERROR("sound::reset_sound()::State is not valid");
    return;
  }
  if (id >= SOUND_ID_MAX or id <= SOUND_ID_UNSPECIFIED) {
    IWARN("sound::reset_sound()::Sound id is out of bound");
    return;
  }
  state->sounds.at(id).play_once = false;
  state->sounds.at(id).played = false;
}

bool sound_system_on_event(i32 code, event_context context) {
  switch (code)
  {
  case EVENT_CODE_PLAY_SOUND:{
    play_sound(static_cast<sound_id>(context.data.i32[0]));
    return true;
  }
  case EVENT_CODE_PLAY_MUSIC:{
    play_music(static_cast<music_id>(context.data.i32[0]));
    return true;
  }
  case EVENT_CODE_RESET_SOUND:{
    reset_sound(static_cast<sound_id>(context.data.i32[0]));
    return true;
  }
  case EVENT_CODE_RESET_MUSIC:{
    reset_music(static_cast<music_id>(context.data.i32[0]));
    return true;
  }
  case EVENT_CODE_PLAY_BUTTON_ON_CLICK:{
    sound_id id = SOUND_ID_BUTTON_ON_CLICK;
    sound_data* data = __builtin_addressof(state->sounds.at(id));
    if (data->play_once && data->played) {
      return true;
    }
    data->play_once = context.data.u16[0];
    data->played = true;
    play_sound(id);
    break;
  }
  case EVENT_CODE_PLAY_FIRE_HIT: {
    sound_id id = SOUND_ID_FIRE_ON_HIT;
    sound_data* data = __builtin_addressof(state->sounds.at(id));

    if (data->play_once && data->played) {
      return true;
    }
    data->play_once = context.data.u16[0];
    data->played = true;
    play_sound(id);
    return true;
  }
  case EVENT_CODE_PLAY_MAIN_MENU_THEME: {
    play_music(MUSIC_ID_MAIN_MENU_THEME);
    return true;
  }
  case EVENT_CODE_PLAY_NIGHT_THEME2: {
    play_music(MUSIC_ID_NIGHT_THEME2);
    return true;
  }
  case EVENT_CODE_PLAY_TRACK5: {
    play_music(MUSIC_ID_TRACK5);
    return true;
  }
  default: return false;
  }

  return false;
}
