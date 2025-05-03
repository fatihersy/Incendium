#include "sound.h"
#include "raylib.h"

#include "core/fmemory.h"
#include "core/event.h"

#if USE_PAK_FORMAT 
#include "tools/pak_parser.h"
#endif

#include "game/game_types.h"

// TODO: Destroy sound system

typedef struct sound_system_state {
  sound_data sounds[SOUND_ID_MAX];
  music_data musics[MUSIC_ID_MAX];
  bool start_trace;
}sound_system_state;

static sound_system_state * state;

void load_sound(const char* file_name, sound_id id);
void load_music(const char* file_name, music_id id);

bool sound_system_on_event(u16 code, event_context context);

#define MAX_SOUND_SLOT 20

/**
 * @brief Depends on pak_parser, init after that
 */
void sound_system_initialize(void) {
  if (state) {
    TraceLog(LOG_WARNING, "sound::sound_system_initialize()::Init called twice");
    return;
  }
  state = (sound_system_state*)allocate_memory_linear(sizeof(sound_system_state), true);

  InitAudioDevice();
  
  load_sound("button_click1.wav", SOUND_ID_BUTTON_ON_CLICK);
  load_sound("deny.wav", SOUND_ID_DENY);
  load_sound("fire_hit.wav", SOUND_ID_FIRE_ON_HIT);
  load_music("main_menu_theme.wav", MUSIC_ID_MAIN_MENU_THEME);
  load_music("night_theme_2.wav", MUSIC_ID_NIGHT_THEME2);
  load_music("Track_#5.wav", MUSIC_ID_TRACK5);

  event_register(EVENT_CODE_PLAY_BUTTON_ON_CLICK, sound_system_on_event);
  event_register(EVENT_CODE_PLAY_FIRE_HIT, sound_system_on_event);
  event_register(EVENT_CODE_PLAY_MAIN_MENU_THEME, sound_system_on_event);
  event_register(EVENT_CODE_PLAY_NIGHT_THEME2, sound_system_on_event);
  event_register(EVENT_CODE_PLAY_TRACK5, sound_system_on_event);
  event_register(EVENT_CODE_PLAY_SOUND, sound_system_on_event);
  event_register(EVENT_CODE_PLAY_MUSIC, sound_system_on_event);
  event_register(EVENT_CODE_RESET_SOUND, sound_system_on_event);
  event_register(EVENT_CODE_RESET_MUSIC, sound_system_on_event);
}

void update_sound_system(void) {
  if (!state) {
    TraceLog(LOG_ERROR, "sound::update_sound_system()::State is not valid");
    return;
  }

  for (int i=0; i<MUSIC_ID_MAX; ++i) {
    UpdateMusicStream(state->musics[i].handle);
  }
}

void load_sound(const char* file_name, sound_id id) {
  file_data file = {};
  Wave wav = {};
  sound_data data = {};

  #if USE_PAK_FORMAT
    file = get_file_data(file_name);
    wav = LoadWaveFromMemory((const char*)file.file_extension, file.data, file.size);
    if (!file.is_initialized) {
      TraceLog(LOG_ERROR, "file:'%s' is not initialized", file.file_name);
      return;
    }
  #else
    wav = LoadWave(TextFormat("%s%s", RESOURCE_PATH, file_name));
  #endif

  data.wav = wav;
  data.file = file;
  data.id = id;
  data.handle = LoadSoundFromWave(wav);
  state->sounds[id] = data;
}

void load_music(const char* file_name, music_id id) {
  file_data file = {};
  Music music = {};

  #if USE_PAK_FORMAT
    file = get_file_data(file_name);
    music = LoadMusicStreamFromMemory((const char*)file.file_extension, file.data, file.size);
    if (!file.is_initialized) {
      TraceLog(LOG_ERROR, "file:'%s' is not initialized", file.file_name);
      return;
    }
  #else
    music = LoadMusicStream(TextFormat("%s%s", RESOURCE_PATH, file_name));
  #endif

  music_data data = {};
  data.file = file;
  data.id = id;
  data.handle = music;
  state->musics[id] = data;
}

void play_sound(sound_id id) {
  if (!state) {
    TraceLog(LOG_ERROR, "sound::play_sound()::State is not valid");
    return;
  }
  if (id >= SOUND_ID_MAX || id <= SOUND_ID_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "sound::play_sound()::Sound id is out of bound");
    return;
  }

  PlaySound(state->sounds[id].handle);
}

void play_music(music_id id) {
  if (!state) {
    TraceLog(LOG_ERROR, "sound::play_music()::State is not valid");
    return;
  }
  if (id >= MUSIC_ID_MAX || id <= MUSIC_ID_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "sound::play_music()::Music id is out of bound");
    return;
  }

  PlayMusicStream(state->musics[id].handle);
}
void reset_music(music_id id) {
  if (!state) {
    TraceLog(LOG_ERROR, "sound::reset_music()::State is not valid");
    return;
  }
  if (id >= MUSIC_ID_MAX || id <= MUSIC_ID_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "sound::reset_music()::Music id is out of bound");
    return;
  }
  state->musics[id].play_once = false;
  state->musics[id].played = false;
  StopMusicStream(state->musics[id].handle);
}
void reset_sound(sound_id id) {
  if (!state) {
    TraceLog(LOG_ERROR, "sound::reset_sound()::State is not valid");
    return;
  }
  if (id >= SOUND_ID_MAX || id <= SOUND_ID_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "sound::reset_sound()::Sound id is out of bound");
    return;
  }
  state->sounds[id].play_once = false;
  state->sounds[id].played = false;
}

bool sound_system_on_event(u16 code, event_context context) {
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
    sound_data* data = &state->sounds[id];
    if (data->play_once && data->played) {
      return true;
    }
    data->play_once = context.data.u16[0];
    data->played = true;
    state->start_trace = true;
    play_sound(id);
    break;
  }
  case EVENT_CODE_PLAY_FIRE_HIT: {
    sound_id id = SOUND_ID_FIRE_ON_HIT;
    sound_data* data = &state->sounds[id];

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

