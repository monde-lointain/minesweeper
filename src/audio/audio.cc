/* audio.cc — SDL_mixer 3.2.2 playback (Stream B.4).
 *
 * Confines all SDL_mixer use to this translation unit. Plays win/explode
 * one-shot effects via fire-and-forget tracks. No per-second tick. */
#include "minesweeper/audio.h"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <stdio.h>
#include <string.h>

/* File-static mixer state. All zero/NULL when audio is unavailable. */
static MIX_Mixer *g_mixer = NULL;
static MIX_Audio *g_win = NULL;
static MIX_Audio *g_explode = NULL;

/* Build "dir/name" into buf, tolerating a trailing slash on dir. */
static void audio_join_path(char *buf, size_t buf_size, const char *dir,
                            const char *name) {
  size_t dir_len = strlen(dir);
  if (dir_len > 0 && dir[dir_len - 1] == '/') {
    snprintf(buf, buf_size, "%s%s", dir, name);
  } else {
    snprintf(buf, buf_size, "%s/%s", dir, name);
  }
}

bool audio_init(const char *dir) {
  if (g_mixer != NULL) {
    return true;
  }

  if (!MIX_Init()) {
    return false;
  }

  g_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
  if (g_mixer == NULL) {
    MIX_Quit();
    return false;
  }

  char path[4096];

  audio_join_path(path, sizeof path, dir, "win.wav");
  g_win = MIX_LoadAudio(g_mixer, path, true);
  if (g_win == NULL) {
    audio_shutdown();
    return false;
  }

  audio_join_path(path, sizeof path, dir, "explode.wav");
  g_explode = MIX_LoadAudio(g_mixer, path, true);
  if (g_explode == NULL) {
    audio_shutdown();
    return false;
  }

  return true;
}

void audio_shutdown(void) {
  if (g_win != NULL) {
    MIX_DestroyAudio(g_win);
    g_win = NULL;
  }
  if (g_explode != NULL) {
    MIX_DestroyAudio(g_explode);
    g_explode = NULL;
  }
  if (g_mixer != NULL) {
    MIX_DestroyMixer(g_mixer);
    g_mixer = NULL;
    MIX_Quit();
  }
}

void audio_play_win(bool enabled) {
  if (!enabled || g_mixer == NULL || g_win == NULL) {
    return;
  }
  MIX_PlayAudio(g_mixer, g_win);
}

void audio_play_explode(bool enabled) {
  if (!enabled || g_mixer == NULL || g_explode == NULL) {
    return;
  }
  MIX_PlayAudio(g_mixer, g_explode);
}
