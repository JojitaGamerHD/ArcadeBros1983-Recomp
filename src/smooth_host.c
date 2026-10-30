/* ARCADE_SMOOTH_VIDEO_V090: observe the original game; never write its RAM. */
#include "smooth_host.h"
#include "smooth_video.h"
#include "cyc_render.h"
#include "cyc_mod.h"
#include <string.h>

enum { ENTITIES = 10 };
typedef struct {
    uint8_t oam[256], group[64], active[33];
    int16_t x[33], y[33];
    uint32_t signature[33], scene;
    int valid;
} PreparedSprites;
static PreparedSprites previous, prepared;
static ArcadeSmoothFrame from, to;
static ArcadeVideoStats stats;
static unsigned part, parts = 1, current_scale = 1;

static int prepare_sprites(PreparedSprites *out) {
    static const unsigned counts[8] = {6,4,2,2,2,2,2,1};
    memset(out, 0, sizeof(*out));
    const unsigned mode = cyc_mod_peek(0x40), phase = cyc_mod_peek(0x41);
    out->scene = (mode << 16) | phase;
    for (unsigned i = 0; i < 256; ++i) out->oam[i] = cyc_mod_peek((uint16_t)(0x200 + i));
    /* Bonus layout and transition scenes retain the exact original picture. */
    if (mode != 4 || !phase || cyc_mod_peek(0x30) || cyc_mod_peek(0x51) ||
        cyc_mod_peek(0x4B0) || cyc_mod_peek(0x6A2) || cyc_mod_peek(0xFD)) return 0;
    const int mario = cyc_mod_peek(0x300) && !cyc_mod_peek(0x316);
    const int luigi = cyc_mod_peek(0x320) && !cyc_mod_peek(0x336);
    if (!mario && !luigi) return 0;
    for (unsigned e = 0; e < ENTITIES; ++e) {
        const uint16_t base = (uint16_t)(0x300 + 0x20 * e);
        const unsigned active = cyc_mod_peek(base);
        if (!active) continue;
        const unsigned mode_draw = cyc_mod_peek(base + 5);
        const unsigned offset = cyc_mod_peek(base + 10);
        if (mode_draw >= 8 || (offset & 3)) return 0;
        const unsigned first = offset / 4, count = counts[mode_draw], group = e + 1;
        if (first + count > 64) return 0;
        out->active[group] = 1;
        out->x[group] = cyc_mod_peek(base + 9);
        out->y[group] = cyc_mod_peek(base + 8);
        out->signature[group] = (active << 24) | ((unsigned)cyc_mod_peek(base + 15) << 16) |
            (mode_draw << 8) | (e < 2 ? cyc_mod_peek(base + 22) : 0);
        for (unsigned j = first; j < first + count; ++j) {
            if (out->group[j]) return 0;
            out->group[j] = (uint8_t)group;
        }
    }
    out->valid = 1;
    return 1;
}

void arcade_video_init(void) {
    memset(&previous, 0, sizeof(previous));
    memset(&prepared, 0, sizeof(prepared));
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    memset(&stats, 0, sizeof(stats));
    part = 0; parts = current_scale = 1;
    cyc_render_capture_background(true);
}

void arcade_video_frame(void) {
    ++stats.frames;
    from.eligible = to.eligible = 0;
    prepare_sprites(&prepared);
    const uint8_t *physical = cyc_render_oam();
    int coherent = previous.valid && prepared.valid && previous.scene == prepared.scene;
    for (unsigned i = 0; coherent && i < 256; ++i) {
        const unsigned mask = (i & 3) == 2 ? 0xE3 : 0xFF;
        if ((previous.oam[i] & mask) != physical[i]) coherent = 0;
    }
    for (unsigned g = 1; coherent && g <= ENTITIES; ++g)
        if (previous.active[g] != prepared.active[g] ||
            previous.signature[g] != prepared.signature[g]) coherent = 0;
    if (coherent && cyc_render_background(to.background)) {
        memcpy(to.native, cyc_frame_argb(), sizeof(to.native));
        memcpy(to.bg_opaque, cyc_frame_bg_opaque(), sizeof(to.bg_opaque));
        for (unsigned i = 0; i < 8192; ++i) to.chr[i] = cyc_render_chr((uint16_t)i);
        for (int i = 0; i < 32; ++i) to.palette[i] = cyc_render_color(i);
        for (int y = 0; y < 240; ++y) {
            to.mask[y] = cyc_render_line_mask(y);
            to.sprite_table[y] = cyc_render_line_sprite_table(y);
            to.sprite16[y] = (uint8_t)cyc_render_line_sprite16(y);
        }
        memcpy(to.oam, prepared.oam, sizeof(to.oam));
        memcpy(to.group, prepared.group, sizeof(to.group));
        memcpy(to.group_active, prepared.active, sizeof(to.group_active));
        memcpy(to.group_x, prepared.x, sizeof(to.group_x));
        memcpy(to.group_y, prepared.y, sizeof(to.group_y));
        to.scene = prepared.scene; to.eligible = 1; to.native_valid = 0;
        memcpy(&from, &to, sizeof(from));
        memcpy(from.oam, physical, sizeof(from.oam));
        memcpy(from.group, previous.group, sizeof(from.group));
        memcpy(from.group_active, previous.active, sizeof(from.group_active));
        memcpy(from.group_x, previous.x, sizeof(from.group_x));
        memcpy(from.group_y, previous.y, sizeof(from.group_y));
        /* Verify the reconstruction against the PPU before replacing sprites.
         * Unknown raster effects always fall back to the untouched native frame. */
        int w, h;
        const uint32_t *check = arcade_smooth_render_pair(&from, &to, 0, 2, &w, &h);
        int exact = check != NULL;
        for (unsigned y = 0; exact && y < 240; ++y)
            for (unsigned x = 0; x < 256; ++x)
                if (check[(y * 6) * (unsigned)w + x * 6] != from.native[y * 256 + x]) {
                    exact = 0; ++stats.reconstruction_mismatch; break;
                }
        from.native_valid = 1;
        if (exact) ++stats.pairs;
        else from.eligible = to.eligible = 0;
    }
    previous = prepared;
}

void arcade_video_phase(void *ctx, unsigned numerator, unsigned denominator) {
    (void)ctx; part = numerator; parts = denominator;
}
unsigned arcade_video_scale(void *ctx) { (void)ctx; return current_scale; }
const uint32_t *arcade_video_present(void *ctx, int *width, int *height) {
    (void)ctx; current_scale = 1;
    if (parts != 2 && parts != 3) return NULL;
    const uint32_t *out = arcade_smooth_render_pair(&from, &to, part, parts, width, height);
    if (out) { current_scale = ARCADE_SMOOTH_SCALE; if (part) ++stats.rendered; }
    else ++stats.native_fallback;
    return out;
}
void arcade_video_stats(ArcadeVideoStats *out) { if (out) *out = stats; }
