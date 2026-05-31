/* audio.h — SDL_mixer playback contract. FROZEN CONTRACT (Stream A).
 *
 * Confines all SDL_mixer use to audio.cc. Plays win/explode only (no tick,
 * matching the released game). See ~/.../winmine/bmp/{win,explode}.wav.
 */
#ifndef MINESWEEPER_AUDIO_H
#define MINESWEEPER_AUDIO_H

#include <stdbool.h>

/* Caller-owned mixer state (was audio.cc file-statics; see design doc amendment
 * 2026-05-31). SDL_mixer types stay confined to audio.cc, so the handles are
 * stored as void* here and C-style-cast in the implementation. All zero/NULL
 * when audio is unavailable. */
struct Audio {
  void* mixer;   /* MIX_Mixer*  */
  void* win;     /* MIX_Audio*  */
  void* explode; /* MIX_Audio*  */
};

/* Open the mixer and load win/explode WAVs from `dir` into `a`. Returns false
 * if audio is unavailable; playback then becomes a no-op (non-fatal). */
bool audio_init(struct Audio* a, const char* dir);
void audio_shutdown(struct Audio* a);

/* Play effects when `enabled` (Settings.sound). No-ops if init failed. */
void audio_play_win(const struct Audio* a, bool enabled);
void audio_play_explode(const struct Audio* a, bool enabled);

#endif /* MINESWEEPER_AUDIO_H */
