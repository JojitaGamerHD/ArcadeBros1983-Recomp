#ifndef ARCADE_NATIVE_TITLE_AUDIO_H
#define ARCADE_NATIVE_TITLE_AUDIO_H

#include <stdint.h>

void arcade_title_audio_set_rom(const char *path);
void arcade_title_audio_restart(void);
const uint32_t *arcade_title_audio_update(int enabled, int volume, int show_demo);
void arcade_title_audio_stop(void);

#endif
