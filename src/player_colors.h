#ifndef ARCADE_PLAYER_COLORS_H
#define ARCADE_PLAYER_COLORS_H
#include <stdint.h>
#include <stdio.h>
#include "recomp_runtime_ui.h"
#ifdef __cplusplus
extern "C" {
#endif
unsigned arcade_colors_current(uint32_t out[2][3]);
void arcade_colors_load(void *ctx, const char *key, const char *value);
void arcade_colors_save(void *ctx, FILE *f);
int arcade_colors_get(void *ctx, const RecompRuntimeUiItem *it, int *value);
int arcade_colors_set(void *ctx, const RecompRuntimeUiItem *it, int value);
int arcade_colors_action(void *ctx, const RecompRuntimeUiItem *it);
int arcade_colors_get_text(void *ctx, const RecompRuntimeUiItem *it, char *out, size_t n);
int arcade_colors_set_text(void *ctx, const RecompRuntimeUiItem *it, const char *value);
/* Sprite indices are captured from the user's local ROM. A missing/invalid
 * frame keeps the last valid pose for the pause-menu reference. */
void arcade_colors_capture_preview(unsigned player, const uint8_t *indices, int width, int height);
int arcade_colors_preview(void *ctx, const RecompRuntimeUiItem *it, RecompRuntimeUiColorPreview *out);
#ifdef __cplusplus
}
#endif
#endif
