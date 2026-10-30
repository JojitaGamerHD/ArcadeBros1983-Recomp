#include "smooth_video.h"

#include <array>
#include <cstdlib>
#include <cstring>

namespace {
constexpr int W = ARCADE_SMOOTH_WIDTH;
constexpr int H = ARCADE_SMOOTH_HEIGHT;
constexpr int S = ARCADE_SMOOTH_SCALE;
constexpr int SW = W * S;
constexpr int SH = H * S;
constexpr unsigned G = ARCADE_SMOOTH_GROUPS;
std::array<uint32_t, SW * SH> canvas;
std::array<uint8_t, SW * SH> occupied;

unsigned group_of(const ArcadeSmoothFrame &f, unsigned slot) {
    const unsigned group = f.group[slot];
    return group <= G ? group : 0;
}

bool visible_slot(const ArcadeSmoothFrame &f, unsigned slot) {
    return unsigned(f.oam[slot * 4]) + 1u < unsigned(H);
}

int wrap_delta(int delta) {
    delta %= W;
    if (delta > W / 2) delta -= W;
    if (delta < -W / 2) delta += W;
    return delta;
}

int wrap_origin(int x) {
    x %= SW;
    return x < 0 ? x + SW : x;
}

/* The first eight OAM entries on the original hardware line remain the
 * visible eight. Interpolation does not alter evaluation, sprite-zero hits,
 * sprite overflow or any CPU/PPU timing. */
void row_visibility(const ArcadeSmoothFrame &f, uint16_t rows[64]) {
    std::memset(rows, 0, sizeof(uint16_t) * 64);
    for (int y = 0; y < H; ++y) {
        unsigned selected = 0;
        for (unsigned slot = 0; slot < 64; ++slot) {
            const int top = int(f.oam[slot * 4]) + 1;
            const int height = f.sprite16[y] ? 16 : 8;
            if (y < top || y >= top + height) continue;
            if (++selected <= 8) rows[slot] |= uint16_t(1u << (y - top));
        }
    }
}

void enlarge(const uint32_t *native) {
    for (int y = 0; y < H; ++y) {
        uint32_t *first = canvas.data() + (y * S) * SW;
        for (int x = 0; x < W; ++x)
            for (int sub = 0; sub < S; ++sub) first[x * S + sub] = native[y * W + x];
        for (int sub = 1; sub < S; ++sub)
            std::memcpy(first + sub * SW, first, SW * sizeof(uint32_t));
    }
}

bool displacement(const ArcadeSmoothFrame &from, const ArcadeSmoothFrame &to,
    int dx[G + 1], int dy[G + 1]) {
    bool seen_from[G + 1] = {};
    bool seen_to[G + 1] = {};
    for (unsigned slot = 0; slot < 64; ++slot) {
        if (visible_slot(from, slot)) seen_from[group_of(from, slot)] = true;
        if (visible_slot(to, slot)) seen_to[group_of(to, slot)] = true;
    }
    for (unsigned group = 1; group <= G; ++group) {
        if (seen_from[group] != seen_to[group]) return false;
        if (!seen_from[group]) continue;
        if (!from.group_active[group] || !to.group_active[group]) return false;
        dx[group] = wrap_delta(int(to.group_x[group]) - int(from.group_x[group]));
        dy[group] = int(to.group_y[group]) - int(from.group_y[group]);
        if (std::abs(dx[group]) > 12 || std::abs(dy[group]) > 12) return false;
    }
    return true;
}

void draw_sprites(const ArcadeSmoothFrame &f, const int dx[G + 1],
    const int dy[G + 1], unsigned numerator, unsigned denominator) {
    uint16_t rows[64];
    row_visibility(f, rows);
    occupied.fill(0);
    for (unsigned slot = 0; slot < 64; ++slot) {
        const uint8_t *sprite = f.oam + slot * 4;
        const int top = int(sprite[0]) + 1;
        if (top >= H) continue;
        const unsigned group = group_of(f, slot);
        const int shift_x = dx[group] * int(numerator) * (S / int(denominator));
        const int shift_y = dy[group] * int(numerator) * (S / int(denominator));
        const int left6 = wrap_origin(int(sprite[3]) * S + shift_x);
        const int top6 = top * S + shift_y;
        const uint8_t attr = sprite[2];
        for (int row = 0; row < 16; ++row) {
            const int source_y = top + row;
            if (source_y >= H || !(rows[slot] & uint16_t(1u << row))) continue;
            const uint8_t mask = f.mask[source_y];
            if (!(mask & 0x10)) continue;
            const bool tall = f.sprite16[source_y] != 0;
            if (!tall && row >= 8) continue;
            int pattern_row = (attr & 0x80) ? ((tall ? 16 : 8) - 1 - row) : row;
            unsigned pattern;
            if (tall) {
                pattern = unsigned(sprite[1] & 1u) * 0x1000u +
                    unsigned(sprite[1] & 0xfeu) * 16u + unsigned(pattern_row / 8) * 16u;
                pattern_row %= 8;
            } else {
                pattern = unsigned(f.sprite_table[source_y] & 0x1000u) + unsigned(sprite[1]) * 16u;
            }
            const uint8_t lo = f.chr[(pattern + unsigned(pattern_row)) & 0x1fffu];
            const uint8_t hi = f.chr[(pattern + unsigned(pattern_row) + 8u) & 0x1fffu];
            for (int column = 0; column < 8; ++column) {
                const int bit = (attr & 0x40) ? column : 7 - column;
                const unsigned color = ((lo >> bit) & 1u) | (((hi >> bit) & 1u) << 1u);
                if (!color) continue;
                const uint32_t argb = f.palette[16u + unsigned(attr & 3u) * 4u + color];
                const int x6 = left6 + column * S;
                const int y6 = top6 + row * S;
                for (int sy = 0; sy < S; ++sy) {
                    const int y = y6 + sy;
                    if (y < 0 || y >= SH) continue;
                    for (int sx = 0; sx < S; ++sx) {
                        const int x = x6 + sx;
                        if (x < 0 || x >= SW || (x < 8 * S && !(mask & 0x04))) continue;
                        const unsigned pixel = unsigned(y * SW + x);
                        if (occupied[pixel]) continue;
                        /* A front OAM sprite wins sprite arbitration even
                         * when an opaque background subsequently hides it. */
                        occupied[pixel] = 1;
                        const unsigned background_pixel = unsigned((y / S) * W + x / S);
                        if (!(attr & 0x20) || !f.bg_opaque[background_pixel]) canvas[pixel] = argb;
                    }
                }
            }
        }
    }
}
} // namespace

extern "C" const uint32_t *arcade_smooth_render_pair(const ArcadeSmoothFrame *from,
    const ArcadeSmoothFrame *to, unsigned numerator, unsigned denominator,
    int *width, int *height) {
    if (width) *width = 0;
    if (height) *height = 0;
    if (!from || !to || !width || !height || !from->eligible || !to->eligible ||
        from->scene != to->scene || !denominator || numerator > denominator ||
        denominator > unsigned(S) || S % denominator != 0) return nullptr;
    int dx[G + 1] = {};
    int dy[G + 1] = {};
    if (!displacement(*from, *to, dx, dy)) return nullptr;
    if (!numerator && from->native_valid) {
        enlarge(from->native);
    } else {
        enlarge(from->background);
        draw_sprites(*from, dx, dy, numerator, denominator);
    }
    *width = SW;
    *height = SH;
    return canvas.data();
}
