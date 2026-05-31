/* audio.cc — SDL_mixer 3.2.2 playback (Stream B.4).
 *
 * Confines all SDL_mixer use to this translation unit. Plays win/explode
 * one-shot effects via fire-and-forget tracks. No per-second tick. */
#include "minesweeper/audio.h"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <stdio.h>
#include <string.h>

#include "minesweeper/util.h"

bool audio_init(struct Audio* a, const char* dir) {
  if (a->mixer != NULL) {
    return true;
  }

  if (!MIX_Init()) {
    return false;
  }

  MIX_Mixer* mixer =
      MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
  if (mixer == NULL) {
    MIX_Quit();
    return false;
  }
  a->mixer = mixer;

  char path[4096];

  util_join_path(path, sizeof path, dir, "win.wav");
  a->win = MIX_LoadAudio(mixer, path, true);
  if (a->win == NULL) {
    audio_shutdown(a);
    return false;
  }

  util_join_path(path, sizeof path, dir, "explode.wav");
  a->explode = MIX_LoadAudio(mixer, path, true);
  if (a->explode == NULL) {
    audio_shutdown(a);
    return false;
  }

  return true;
}

void audio_shutdown(struct Audio* a) {
  if (a->win != NULL) {
    MIX_DestroyAudio((MIX_Audio*)a->win);
    a->win = NULL;
  }
  if (a->explode != NULL) {
    MIX_DestroyAudio((MIX_Audio*)a->explode);
    a->explode = NULL;
  }
  if (a->mixer != NULL) {
    MIX_DestroyMixer((MIX_Mixer*)a->mixer);
    a->mixer = NULL;
    MIX_Quit();
  }
}

void audio_play_win(const struct Audio* a, bool enabled) {
  if (!enabled || a->mixer == NULL || a->win == NULL) {
    return;
  }
  MIX_PlayAudio((MIX_Mixer*)a->mixer, (MIX_Audio*)a->win);
}

void audio_play_explode(const struct Audio* a, bool enabled) {
  if (!enabled || a->mixer == NULL || a->explode == NULL) {
    return;
  }
  MIX_PlayAudio((MIX_Mixer*)a->mixer, (MIX_Audio*)a->explode);
}
