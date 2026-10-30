#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* One producer thread; PCM overlays at the game's 48000 Hz mono output. */
int arcade_music_load_default(void);
int arcade_music_load_utf8(const char *path);
void arcade_music_reset(void);
void arcade_music_mix(int16_t *samples, size_t count, int enabled, int volume_percent);
size_t arcade_music_frame_count(void);
size_t arcade_music_cursor(void);
#ifdef __cplusplus
}
#endif
