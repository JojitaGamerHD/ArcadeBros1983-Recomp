#ifndef ARCADE_GRID_H
#define ARCADE_GRID_H

#include <stdint.h>

static inline int arcade_grid_alpha(int intensity)
{
    if (intensity <= 0) return 0;
    if (intensity >= 100) return 153;
    return (intensity * 153 + 50) / 100;
}

static inline int arcade_grid_visible(int width, int height, int source_width, int source_height)
{
    /* Leave room for at least one unmasked output pixel in every texel. */
    return source_width > 0 && source_height > 0 &&
        width / 2 >= source_width && height / 2 >= source_height;
}

static inline int arcade_grid_offset(int length, int index, int source_length)
{
    return (int)((int64_t)length * index / source_length);
}

#endif
