/* audio.h — SDL_mixer playback contract. FROZEN CONTRACT (Stream A).
 *
 * Confines all SDL_mixer use to audio.cc. Plays win/explode only (no tick,
 * matching the released game). See ~/.../winmine/bmp/{win,explode}.wav.
 */
#ifndef MINESWEEPER_AUDIO_H
#define MINESWEEPER_AUDIO_H

#include <stdbool.h>

/* Open the mixer and load win/explode WAVs from `dir`. Returns false if audio
 * is unavailable; playback then becomes a no-op (non-fatal). */
bool audio_init(const char *dir);
void audio_shutdown(void);

/* Play effects when `enabled` (Settings.sound). No-ops if init failed. */
void audio_play_win(bool enabled);
void audio_play_explode(bool enabled);

#endif /* MINESWEEPER_AUDIO_H */
