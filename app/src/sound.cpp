#include "sound.h"
#include "raylib.h"

#include "core/fmemory.h"
#include "core/event.h"
#include "core/logger.h"
#include "core/ftime.h"

#if USE_PAK_FORMAT 
  #include "tools/pak_parser.h"
#endif

// TODO: Destroy sound system

typedef struct soundgroup {
  std::vector<sound_id> queue;
  size_t current_index;

  bool lock_after_play;
  bool loop_list;
	bool loop_one;
  bool mix_list;
  bool locked;

	soundgroup(void) {
		this->queue = std::vector<sound_id>();
		this->current_index = 0u;
    this->lock_after_play = false;
		this->loop_list = false;
		this->loop_one = false;
		this->mix_list = false;
    this->locked = false;
	}
	soundgroup(std::vector<sound_id> _queue, bool _lock_after_play, bool _mix_list, bool _loop_list = false, bool _loop_one = false) : soundgroup() {
    this->lock_after_play = _lock_after_play;
		this->queue = _queue;
		this->loop_list = _loop_list;
		this->loop_one = _loop_one;
		this->mix_list = _mix_list;
	}
} soundgroup;

typedef struct sound_system_state {
  std::array<sound_data, SOUND_ID_MAX> sounds;
  std::array<music_data, MUSIC_ID_MAX> musics;
  std::array<soundgroup, SOUNDGROUP_ID_MAX> sound_groups;

  #if not USE_PAK_FORMAT
  std::array<file_buffer, SOUND_ID_MAX> sound_datas; 
  std::array<file_buffer, MUSIC_ID_MAX> music_datas; 
  #endif

  playlist_control_system_state * current_playlist;

  sound_system_state(void) {
    this->current_playlist = nullptr;
  }
}sound_system_state;

static sound_system_state * state = nullptr;

#define ASSERT_NOT_STATE(FUNCTION, RETURN) do { if (not state or state == nullptr) { \
    IERROR("sound::" FUNCTION "::State is not valid");\
    RETURN \
} } while(0)

#define CREATE_SOUND_GROUP(GROUP_ID, LOCK, MIX_LIST, LOOP_LIST, LOOP_ONE, ...)\
  do { state->sound_groups.at(GROUP_ID) = soundgroup(std::vector<sound_id>({__VA_ARGS__}), LOCK, MIX_LIST, LOOP_LIST, LOOP_ONE); } while(0)


void load_sound_pak(pak_file_id pak_id, i32 file_id, sound_id id, std::array<f32, 2> pitch_range = {1.f, 1.f});
void load_music_pak(pak_file_id pak_id, i32 file_id, music_id id);
void load_sound_disk(const char * filename, sound_id id, std::array<f32, 2> pitch_range = {1.f, 1.f});
void load_music_disk(const char * filename, music_id id);
bool media_prev(playlist_control_system_state * playlist_ptr, bool play_now);
bool media_next(playlist_control_system_state * playlist_ptr, bool play_now);
void media_play(playlist_control_system_state * playlist_ptr);
void media_pause(playlist_control_system_state * playlist_ptr);
void media_stop(playlist_control_system_state * playlist_ptr);
void update_playlist(playlist_control_system_state * playlist_ptr);
void play_soundgroup_sound(const soundgroup * group, bool random_pitch);

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
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_BTN_CLICK_1,      SOUND_ID_BUTTON_ON_CLICK1);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_BTN_CLICK_2,      SOUND_ID_BUTTON_ON_CLICK2);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_BTN_CLICK_3,      SOUND_ID_BUTTON_ON_CLICK3);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_DENY1,            SOUND_ID_DENY1);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_DENY2,            SOUND_ID_DENY2);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_EXP_PICKUP,             SOUND_ID_EXP_PICKUP);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_HEALTH_PICKUP,          SOUND_ID_HEALTH_PICKUP);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_LEVEL_UP,         SOUND_ID_LEVEL_UP);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_ZAP_1,            SOUND_ID_ZAP1);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_ZAP_2,            SOUND_ID_ZAP2);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_ZAP_3,            SOUND_ID_ZAP3);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_ZAP_4,            SOUND_ID_ZAP4);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_ZOMBIE_DIE_1,     SOUND_ID_ZOMBIE_DIE1);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_ZOMBIE_DIE_2,     SOUND_ID_ZOMBIE_DIE2);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SOUND_ZOMBIE_DIE_3,     SOUND_ID_ZOMBIE_DIE3);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SPIN_SFX,              SOUND_ID_SPIN_SFX);
  load_sound_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SPIN_RESULT,           SOUND_ID_SPIN_RESULT);
  load_music_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_THEME_MAINMENU_1,      MUSIC_ID_MAINMENU_THEME1);
  load_music_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_THEME_MAINMENU_2,      MUSIC_ID_MAINMENU_THEME2);
  load_music_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_THEME_MAINMENU_3,      MUSIC_ID_MAINMENU_THEME3);
  load_music_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_THEME_MAP_SELECTION_1, MUSIC_ID_MAP_SELECTION_THEME1);
  load_music_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_THEME_MAP_SELECTION_2, MUSIC_ID_MAP_SELECTION_THEME2);
  load_music_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_THEME_INGAME_PLAY_1,   MUSIC_ID_INGAME_PLAY_THEME1);
  load_music_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_THEME_INGAME_PLAY_2,   MUSIC_ID_INGAME_PLAY_THEME2);
  load_music_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_THEME_INGAME_PLAY_3,   MUSIC_ID_INGAME_PLAY_THEME3);
  load_music_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_THEME_INGAME_PLAY_4,   MUSIC_ID_INGAME_PLAY_THEME4);
  #else
  load_sound_disk("button_click1.wav", SOUND_ID_BUTTON_ON_CLICK1);
  load_sound_disk("button_click2.wav", SOUND_ID_BUTTON_ON_CLICK2);
  load_sound_disk("button_click3.wav", SOUND_ID_BUTTON_ON_CLICK3);
  load_sound_disk("coin_pickup.mp3", SOUND_ID_COIN_PICKUP);
  load_sound_disk("deny1.wav", SOUND_ID_DENY1);
  load_sound_disk("deny2.wav", SOUND_ID_DENY2);
  load_sound_disk("exp_pickup.wav", SOUND_ID_EXP_PICKUP);
  load_sound_disk("health_pickup.mp3", SOUND_ID_HEALTH_PICKUP);
  load_sound_disk("level_up.mp3", SOUND_ID_LEVEL_UP);
  load_sound_disk("zap1.wav", SOUND_ID_ZAP1, {1.f, 1.5f});
  load_sound_disk("zap2.wav", SOUND_ID_ZAP2, {1.f, 1.5f});
  load_sound_disk("zap3.wav", SOUND_ID_ZAP3, {1.f, 1.5f});
  load_sound_disk("zap4.wav", SOUND_ID_ZAP4, {1.f, 1.5f});
  load_sound_disk("zombie_die1.wav", SOUND_ID_ZOMBIE_DIE1);
  load_sound_disk("zombie_die2.wav", SOUND_ID_ZOMBIE_DIE2);
  load_sound_disk("zombie_die3.wav", SOUND_ID_ZOMBIE_DIE3);
  load_sound_disk("spin_sfx.wav",    SOUND_ID_SPIN_SFX);
  load_sound_disk("spin_result.wav", SOUND_ID_SPIN_RESULT);
  load_music_disk("mainmenu_theme1.wav", MUSIC_ID_MAINMENU_THEME1);
  load_music_disk("mainmenu_theme2.wav", MUSIC_ID_MAINMENU_THEME2);
  load_music_disk("mainmenu_theme3.wav", MUSIC_ID_MAINMENU_THEME3);
  load_music_disk("map_selection1.wav", MUSIC_ID_MAP_SELECTION_THEME1);
  load_music_disk("map_selection2.wav", MUSIC_ID_MAP_SELECTION_THEME2);
  load_music_disk("ingame_play_theme1.ogg", MUSIC_ID_INGAME_PLAY_THEME1);
  load_music_disk("ingame_play_theme2.ogg", MUSIC_ID_INGAME_PLAY_THEME2);
  load_music_disk("ingame_play_theme3.ogg", MUSIC_ID_INGAME_PLAY_THEME3);
  load_music_disk("ingame_play_theme4.ogg", MUSIC_ID_INGAME_PLAY_THEME4);
  #endif

  event_register(EVENT_CODE_PLAY_SOUND, sound_system_on_event);
  event_register(EVENT_CODE_PLAY_SOUND_GROUP, sound_system_on_event);
  event_register(EVENT_CODE_PLAY_MUSIC, sound_system_on_event);
  event_register(EVENT_CODE_RESET_SOUND, sound_system_on_event);
  event_register(EVENT_CODE_RESET_MUSIC, sound_system_on_event);

  CREATE_SOUND_GROUP(SOUNDGROUP_ID_BUTTON_ON_CLICK, false, true, false, false, SOUND_ID_BUTTON_ON_CLICK1, SOUND_ID_BUTTON_ON_CLICK2, SOUND_ID_BUTTON_ON_CLICK3);
  CREATE_SOUND_GROUP(SOUNDGROUP_ID_DENY, false, true, false, false, SOUND_ID_DENY1, SOUND_ID_DENY2);
  CREATE_SOUND_GROUP(SOUNDGROUP_ID_ZAP, false, true, false, false, SOUND_ID_ZAP1, SOUND_ID_ZAP2, SOUND_ID_ZAP3, SOUND_ID_ZAP4);
  CREATE_SOUND_GROUP(SOUNDGROUP_ID_ZOMBIE_DIE, false, true, false, false, SOUND_ID_ZOMBIE_DIE1, SOUND_ID_ZOMBIE_DIE2, SOUND_ID_ZOMBIE_DIE3);

  return true;
}

void update_sound_system(void) {
  ASSERT_NOT_STATE("update_sound_system()", { return; });

  if (state->current_playlist and state->current_playlist != nullptr) {
    if (state->current_playlist->play) {
      update_playlist(state->current_playlist);
    }
  }
}

void load_sound_pak([[__maybe_unused__]] pak_file_id pak_file, [[__maybe_unused__]] i32 file_id, [[__maybe_unused__]] sound_id id, [[__maybe_unused__]] std::array<f32, 2> pitch_range) {
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
  
  data.wav = ZERO_WAV;
  data.file = file;
  data.id = id;
  data.handle = LoadSoundFromWave(wav);
  data.pitch_range = pitch_range;
  UnloadWave(wav);

  state->sounds.at(id) = data;
  #endif
}
void load_sound_disk([[__maybe_unused__]] const char * filename, [[__maybe_unused__]] sound_id id, [[__maybe_unused__]] std::array<f32, 2> pitch_range) {
  #if not USE_PAK_FORMAT
  const file_buffer * file = __builtin_addressof(state->sound_datas.at(id));
  Wave wav;
  sound_data data;

  const char * path = TextFormat("%s%s", RESOURCE_PATH, filename);
  if (not FileExists(path)) {
    IWARN("sound::load_sound_disk()::File %s not found", filename);
    return;
  }
  wav = LoadWave(path);

  data.wav = wav;
  data.file = file;
  data.id = id;
  data.handle = LoadSoundFromWave(wav);
  data.pitch_range = pitch_range;

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

void load_music_disk([[__maybe_unused__]] const char * filename, [[__maybe_unused__]] music_id id) {
  #if not USE_PAK_FORMAT
  const file_buffer * file = __builtin_addressof(state->sound_datas.at(id));
  Music music;

  const char * path = TextFormat("%s%s", RESOURCE_PATH, filename);
  if (not FileExists(path)) {
    IWARN("sound::load_music_disk()::File %s not found", filename);
    return;
  }
  music = LoadMusicStream(path);
  
  music_data data = music_data();
  data.file = file;
  data.id = id;
  data.handle = music;
  state->musics.at(id) = data;
  #endif
}

void play_sound(sound_id id, bool random_pitch) {
  ASSERT_NOT_STATE("play_sound()", { return; });

  if (id >= SOUND_ID_MAX or id <= SOUND_ID_UNSPECIFIED) {
    IWARN("sound::play_sound()::Sound id is out of bound");
    return;
  }
  sound_data& sound = state->sounds.at(id);

  f32 pitch = 1.f;
  if (random_pitch) {
    const i32 low = sound.pitch_range.at(0) * 10.f;
    const i32 high = sound.pitch_range.at(1) * 10.f;
  
    pitch = get_random(low, high) * .1f;
  }

  SetSoundPitch(sound.handle, pitch);

  PlaySound(sound.handle);
}

void play_music(music_id id) {
  ASSERT_NOT_STATE("play_music()", { return; });

  if (id >= MUSIC_ID_MAX or id <= MUSIC_ID_UNSPECIFIED) {
    IWARN("sound::play_music()::Music id is out of bound");
    return;
  }

  PlayMusicStream(state->musics.at(id).handle);
}
void reset_music(music_id id) {
  ASSERT_NOT_STATE("reset_music()", { return; });

  if (id >= MUSIC_ID_MAX or id <= MUSIC_ID_UNSPECIFIED) {
    IWARN("sound::reset_music()::Music id is out of bound");
    return;
  }
  state->musics.at(id).play_once = false;
  state->musics.at(id).played = false;
  StopMusicStream(state->musics.at(id).handle);
}
void reset_sound(sound_id id) {
  ASSERT_NOT_STATE("reset_sound()", { return; });

  if (id >= SOUND_ID_MAX or id <= SOUND_ID_UNSPECIFIED) {
    IWARN("sound::reset_sound()::Sound id is out of bound");
    return;
  }
  state->sounds.at(id).play_once = false;
  state->sounds.at(id).played = false;
}

bool media_prev(playlist_control_system_state * playlist_ptr, bool play_now) {
  ASSERT_NOT_STATE("media_prev()", { return false; });
  if (not playlist_ptr or playlist_ptr == nullptr or playlist_ptr->queue.empty()) {
    return false;
  }

  playlist_ptr->current_index = playlist_ptr->current_index > 0u ? playlist_ptr->current_index - 1 : 0u;

  if (play_now) {
   media_play(playlist_ptr);
  }
  return true;
}
bool media_next(playlist_control_system_state * playlist_ptr, bool play_now) {
  ASSERT_NOT_STATE("media_next()", { return false; });
  if (not playlist_ptr or playlist_ptr == nullptr or playlist_ptr->queue.empty()) {
    return false;
  }

  if (playlist_ptr->mix_list) {
    playlist_ptr->current_index = get_random(0, static_cast<i32>(playlist_ptr->queue.size() - 1u));
  }
  else {
    if (playlist_ptr->loop_one) {
      playlist_ptr->loop_one = false;
    }
    if (playlist_ptr->loop_list) {
      const size_t last_item_index = playlist_ptr->queue.size() - 1u;
      size_t current_index = playlist_ptr->current_index;

      playlist_ptr->current_index = current_index < last_item_index ? ++current_index : 0u;
    }
    else {
      playlist_ptr->current_index = playlist_ptr->current_index <= playlist_ptr->queue.size() - 2u ? playlist_ptr->current_index++ : playlist_ptr->queue.size() - 1u; 
    }
  }
  
  if (play_now) {
   media_play(playlist_ptr);
  }
  return true;
}
void media_play(playlist_control_system_state * playlist_ptr) {
  ASSERT_NOT_STATE("media_play()", { return; });
  if (not playlist_ptr or playlist_ptr == nullptr or playlist_ptr->queue.empty()) {
    return;
  }
  if (state->current_playlist and state->current_playlist != nullptr and state->current_playlist != playlist_ptr) {
    media_stop(state->current_playlist);
  }
  state->current_playlist = playlist_ptr;

  state->current_playlist->current_index = state->current_playlist->current_index < state->current_playlist->queue.size() ? state->current_playlist->current_index : 0u;

  PlayMusicStream(state->current_playlist->queue.at(state->current_playlist->current_index).handle);
  state->current_playlist->play = true;
}
void media_pause(playlist_control_system_state * playlist_ptr) {
  ASSERT_NOT_STATE("media_pause()", { return; });
  if (not playlist_ptr or playlist_ptr == nullptr or playlist_ptr->queue.empty()) {
    return;
  }
  for (music_data& music_data : playlist_ptr->queue) {
    PauseMusicStream(music_data.handle);
  }
  state->current_playlist->play = false;
}
void media_stop(playlist_control_system_state * playlist_ptr) {
  ASSERT_NOT_STATE("media_stop()", { return; });
  if (not playlist_ptr or playlist_ptr == nullptr or playlist_ptr->queue.empty()) {
    return;
  }
  for (music_data& music_data : playlist_ptr->queue) {
    StopMusicStream(music_data.handle);
  }
  state->current_playlist->play = false;
}

playlist_control_system_state create_playlist(playlist_preset preset) {
  ASSERT_NOT_STATE("create_playlist()", { return playlist_control_system_state(); });

  switch (preset) {
    case PLAYLIST_PRESET_EMPTY: {
      return playlist_control_system_state(std::vector<music_data>(), __builtin_addressof(state->musics), media_prev, media_next, media_pause, media_play, media_stop, false, false, false);
    }
    case PLAYLIST_PRESET_MAINMENU_LIST: {
      return playlist_control_system_state(std::vector<music_data>({
          state->musics.at(MUSIC_ID_MAINMENU_THEME1),
          state->musics.at(MUSIC_ID_MAINMENU_THEME2),
          state->musics.at(MUSIC_ID_MAINMENU_THEME3)
        }), 
        __builtin_addressof(state->musics), media_prev, media_next, media_pause, media_play, media_stop, true, false, false
      );
    }
    case PLAYLIST_PRESET_MAP_SELECTION_LIST: {
      return playlist_control_system_state(std::vector<music_data>({
          state->musics.at(MUSIC_ID_MAP_SELECTION_THEME1),
          state->musics.at(MUSIC_ID_MAP_SELECTION_THEME2)
        }), __builtin_addressof(state->musics), media_prev, media_next, media_pause, media_play, media_stop, true, false, false
      );
    }
    case PLAYLIST_PRESET_INGAME_PLAY_LIST: {
      return playlist_control_system_state(std::vector<music_data>({
          state->musics.at(MUSIC_ID_INGAME_PLAY_THEME1),
          state->musics.at(MUSIC_ID_INGAME_PLAY_THEME2),
          state->musics.at(MUSIC_ID_INGAME_PLAY_THEME3),
          state->musics.at(MUSIC_ID_INGAME_PLAY_THEME4)
        }), __builtin_addressof(state->musics), media_prev, media_next, media_pause, media_play, media_stop, true, false, false
      );
    }
    default: {
      IWARN("sound::create_playlist()::Preset id is out of bound");
      return playlist_control_system_state();
    }
  }

  IERROR("sound::create_playlist()::Function ended unexpectedly");
  return playlist_control_system_state();
}

void update_playlist(playlist_control_system_state * playlist_ptr) {
  if (not playlist_ptr or playlist_ptr == nullptr or playlist_ptr->queue.empty()) {
    return;
  }
  if (state->current_playlist->current_index >= state->current_playlist->queue.size()) {
    media_stop(state->current_playlist);
    state->current_playlist->current_index = 0;
    media_play(state->current_playlist);
  }
  Music& music = state->current_playlist->queue.at(state->current_playlist->current_index).handle;
  f32 time_player = GetMusicTimePlayed(music) / GetMusicTimeLength(music);
  if (time_player <= 0.99f and time_player >= 0.f) {
    UpdateMusicStream(music);
    return;
  }
  
  if (state->current_playlist->loop_one) {
    StopMusicStream(music);
    PlayMusicStream(music);
  }
  else {
    media_next(state->current_playlist, true);
  }
}

void play_soundgroup_sound(soundgroup * group, bool random_pitch) {
  ASSERT_NOT_STATE("play_soundgroup_sound()", { return; });
  if (not group and group == nullptr) {
    return;
  }
  if (group->locked) {
    return;
  }

  if (group->mix_list) {
    group->current_index = get_random(0, static_cast<i32>(group->queue.size() - 1u));
  }
  else {
    if (group->loop_one) {
      group->loop_one = false;
    }
    if (group->loop_list) {
      const size_t last_item_index = group->queue.size() - 1u;
      size_t current_index = group->current_index;

      group->current_index = current_index < last_item_index ? ++current_index : 0u;
    }
    else {
      group->current_index = group->current_index <= group->queue.size() - 2u ? group->current_index++ : group->queue.size() - 1u; 
    }
  }
  if (group->lock_after_play) {
    group->locked = true;
  }
  
  play_sound(group->queue.at(group->current_index), random_pitch);
}

bool sound_system_on_event(i32 code, event_context context) {
  ASSERT_NOT_STATE("sound_system_on_event()", { return false; });

  switch (code)
  {
    case EVENT_CODE_PLAY_SOUND:{
      play_sound(static_cast<sound_id>(context.data.i32[0]), static_cast<bool>(context.data.i32[1]));
      return true;
    }
    case EVENT_CODE_PLAY_SOUND_GROUP:{
      soundgroup * group = __builtin_addressof(state->sound_groups.at(static_cast<soundgroup_id>(context.data.i32[0])));
      play_soundgroup_sound(group, static_cast<bool>(context.data.i32[1]));
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
    case EVENT_CODE_RESET_SOUND_GROUP:{
      soundgroup * group = __builtin_addressof(state->sound_groups.at(static_cast<soundgroup_id>(context.data.i32[0])));
      group->locked = false;
      return true;
    }
    case EVENT_CODE_RESET_MUSIC:{
      reset_music(static_cast<music_id>(context.data.i32[0]));
      return true;
    }
    default: {
      IWARN("sound::sound_system_on_event()::Unsupported event code");
      return false;
    }
  }

  IERROR("sound::sound_system_on_event()::Function ended unexpectedly");
  return false;
}
