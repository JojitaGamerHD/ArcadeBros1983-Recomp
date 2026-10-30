#ifndef ARCADE_SMOOTH_VIDEO_H
#define ARCADE_SMOOTH_VIDEO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    ARCADE_SMOOTH_WIDTH = 256,
    ARCADE_SMOOTH_HEIGHT = 240,
    ARCADE_SMOOTH_SCALE = 6,
    ARCADE_SMOOTH_GROUPS = 32,
    ARCADE_SMOOTH_PIXELS = 256 * 240
};

/* Presentation-only data. All coordinates use native NES pixels. OAM Y is
 * the hardware byte (the sprite starts at Y+1); group_y is a normal anchor.
 * group[slot] is a stable identity 1..32, or 0 for a static/untracked sprite.
 * group_active and group_x/y must describe the same displayed OAM as oam[].
 * Anchors may wrap modulo 256 horizontally. They must not be guessed from a
 * live game state one frame newer than the captured OAM.
 *
 * The source/target pair can be previous/current pictures or a verified
 * current/next OAM pair. This module neither reads nor changes machine state.
 * CHR is the mapped $0000..$1fff image; palette contains opaque ARGB colors.
 * bg_opaque contains the exact native background coverage, not its color.
 * Set eligible=0 or change scene to request a native renderer fallback.
 */
typedef struct ArcadeSmoothFrame {
    uint32_t background[ARCADE_SMOOTH_PIXELS];
    uint32_t native[ARCADE_SMOOTH_PIXELS];
    uint8_t bg_opaque[ARCADE_SMOOTH_PIXELS];
    uint8_t oam[256];
    uint8_t chr[8192];
    uint32_t palette[32];
    uint8_t mask[240];
    uint16_t sprite_table[240];
    uint8_t sprite16[240];
    uint8_t group[64];
    uint8_t group_active[ARCADE_SMOOTH_GROUPS + 1];
    int16_t group_x[ARCADE_SMOOTH_GROUPS + 1];
    int16_t group_y[ARCADE_SMOOTH_GROUPS + 1];
    uint32_t scene;
    int eligible;
    int native_valid;
} ArcadeSmoothFrame;

/* Returns an opaque 1536x1440 ARGB8888 canvas owned by this module, valid
 * until the next call, or NULL to request native fallback. Rational alpha
 * supports 0, 1/2, 1/3 and 2/3 without coordinate rounding to NES pixels.
 * The module is single-threaded, just like the producer/presentation host.
 */
const uint32_t *arcade_smooth_render_pair(const ArcadeSmoothFrame *from,
    const ArcadeSmoothFrame *to, unsigned numerator, unsigned denominator,
    int *width, int *height);

#ifdef __cplusplus
}
#endif
#endif
