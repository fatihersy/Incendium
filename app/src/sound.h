#ifndef SOUND_H
#define SOUND_H

#include "defines.h"
#include "raylib.h"

#ifndef ZERO_AUDIO_STREAM
  #define ZERO_AUDIO_STREAM (AudioStream {nullptr, nullptr, 0u, 0u, 0u})
#endif

#ifndef ZERO_SOUND
  #define ZERO_SOUND (Sound {ZERO_AUDIO_STREAM, 0u})
#endif

#ifndef ZERO_MUSIC
  #define ZERO_MUSIC (Music {ZERO_AUDIO_STREAM, 0u, false, 0, nullptr })
#endif

#ifndef ZERO_WAV
  #define ZERO_WAV (Wave {0u, 0u, 0u, 0u, nullptr})
#endif

typedef enum playlist_preset {
	PLAYLIST_PRESET_UNDEFINED,
	PLAYLIST_PRESET_EMPTY,
	PLAYLIST_PRESET_MAINMENU_LIST,
	PLAYLIST_PRESET_MAP_SELECTION_LIST,
	PLAYLIST_PRESET_INGAME_PLAY_LIST,
	PLAYLIST_PRESET_MAX,
} playlist_preset;

typedef struct music_data {
  music_id id;
  Music handle;
  const file_buffer * file;

  bool play_once;
  bool played;
  music_data(void) {
    this->id = MUSIC_ID_UNSPECIFIED;
    this->handle = ZERO_MUSIC;
    this->file = nullptr;
    this->play_once = false;
    this->played = false;
  }
}music_data;

typedef struct sound_data {
  sound_id id;
  Sound handle;
  Wave wav;
  const file_buffer * file;

  bool play_once;
  bool played;
  sound_data(void) {
    this->id = SOUND_ID_UNSPECIFIED;
    this->handle = ZERO_SOUND;
    this->wav = ZERO_WAV;
    this->file = nullptr;
    this->play_once = false;
    this->played = false;
  }
}sound_data;

typedef struct playlist_control_system_state {
  std::vector<music_data> queue;
  size_t current_index;

  bool loop_list;
	bool loop_one;
  bool mix_list;
	bool play;

	const std::array<music_data, MUSIC_ID_MAX> * library;

	bool (*media_prev )(playlist_control_system_state * playlist_ptr, bool play_now);
	bool (*media_next )(playlist_control_system_state * playlist_ptr, bool play_now);
	void (*media_pause)(playlist_control_system_state * playlist_ptr);
	void (*media_play )(playlist_control_system_state * playlist_ptr);
	void (*media_stop )(playlist_control_system_state * playlist_ptr);

	playlist_control_system_state(void) {
		this->queue = std::vector<music_data>();
		this->current_index = 0u;
		this->loop_list = false;
		this->loop_one = false;
		this->mix_list = false;
		this->play = false;
		this->library = nullptr;
		this->media_prev  = nullptr;
		this->media_next  = nullptr;
		this->media_pause = nullptr;
		this->media_play  = nullptr;
		this->media_stop  = nullptr;
	}
	playlist_control_system_state(
		std::vector<music_data> _queue, 
		const std::array<music_data, MUSIC_ID_MAX> * _library,
		bool (*_media_prev )(playlist_control_system_state * playlist_ptr, bool play_now),
		bool (*_media_next )(playlist_control_system_state * playlist_ptr, bool play_now),
		void (*_media_pause)(playlist_control_system_state * playlist_ptr),
		void (*_media_play )(playlist_control_system_state * playlist_ptr),
		void (*_media_stop )(playlist_control_system_state * playlist_ptr),
		bool _loop_list,
		bool _loop_one,
		bool _mix_list) : playlist_control_system_state() 
	{
		this->queue = _queue;
		this->loop_list = _loop_list;
		this->loop_one = _loop_one;
		this->mix_list = _mix_list;
		this->library = _library;
		this->media_prev  = _media_prev;
		this->media_next  = _media_next;
		this->media_pause = _media_pause;
		this->media_play  = _media_play;
		this->media_stop  = _media_stop;
	}
} playlist_control_system_state;

bool sound_system_initialize(void);

void update_sound_system(void);

void play_sound(sound_id id);
void play_music(music_id id);
void reset_music(music_id id);
void reset_sound(sound_id id);

playlist_control_system_state create_playlist(playlist_preset preset = PLAYLIST_PRESET_EMPTY); 

#endif
