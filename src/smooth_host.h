#ifndef ARCADE_SMOOTH_HOST_H
#define ARCADE_SMOOTH_HOST_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    unsigned frames, pairs, rendered, native_fallback, reconstruction_mismatch;
} ArcadeVideoStats;
void arcade_video_init(void);
void arcade_video_frame(void);
void arcade_video_phase(void *ctx, unsigned numerator, unsigned denominator);
unsigned arcade_video_scale(void *ctx);
const uint32_t *arcade_video_present(void *ctx, int *width, int *height);
void arcade_video_stats(ArcadeVideoStats *out);
#ifdef __cplusplus
}
#endif
#endif
