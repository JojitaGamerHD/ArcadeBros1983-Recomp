/* ARCADE_SMOOTH_VIDEO_V090: observe the original game; never write its RAM. */
#include "smooth_host.h"
#include "smooth_video.h"
#include "player_colors.h"
#include "player_sprite.h"
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
static uint32_t colored_native[256*240];

static unsigned observe_sprite_owner(unsigned slot, const uint8_t entry[4], void *ctx) {
    (void)ctx;
    ArcadeSpriteLayout layout;
    memset(&layout,0,sizeof layout);
    /* Identify the scene at upload time, including the retained phase-end
     * picture and the bonus results' separate, fixed portrait table. */
    arcade_sprite_scene(&layout,cyc_mod_peek(0x40),cyc_mod_peek(0x3B),cyc_mod_peek(0x4B4),
                        cyc_mod_peek(0x30),cyc_mod_peek(0x51));
    if (!layout.gameplay && !layout.bonus_results) return 0;
    for (unsigned e=0;e<ENTITIES;++e) {
        uint16_t base=(uint16_t)(0x300+0x20*e);
        layout.active[e]=cyc_mod_peek(base);
        layout.draw[e]=cyc_mod_peek(base+5);
        layout.offset[e]=cyc_mod_peek(base+10);
        if (e<2) layout.state[e]=cyc_mod_peek(base+22);
    }
    if (layout.bonus_results)
        for (unsigned i=0;i<12*4;++i) layout.shadow[i]=cyc_mod_peek((uint16_t)(0x200+i));
    for (unsigned i=0;i<4;++i) layout.shadow[slot*4+i]=cyc_mod_peek((uint16_t)(0x200+slot*4+i));
    return arcade_player_sprite_owner(&layout,slot,entry);
}

static void capture_player_preview(unsigned player) {
    const uint8_t *oam=cyc_render_oam();
    int left=256, top=240, right=0, bottom=0;
    for (unsigned slot=0;slot<64;++slot) {
        if (cyc_render_sprite_owner(slot)!=player+1 || oam[slot*4]>=0xEF) continue;
        const uint8_t *s=oam+slot*4;
        int y=s[0]+1, height=cyc_render_line_sprite16(y)?16:8;
        if (s[3]<left) left=s[3]; if (y<top) top=y;
        if (s[3]+8>right) right=s[3]+8; if (y+height>bottom) bottom=y+height;
    }
    int width=right-left, height=bottom-top;
    if (width<=0 || width>32 || height<=0 || height>32) return;
    uint8_t indices[32*32]={0};
    for (unsigned slot=0;slot<64;++slot) {
        if (cyc_render_sprite_owner(slot)!=player+1 || oam[slot*4]>=0xEF) continue;
        const uint8_t *s=oam+slot*4; int y=s[0]+1;
        int tall=cyc_render_line_sprite16(y), rows=tall?16:8;
        for (int row=0;row<rows;++row) {
            int r=(s[2]&0x80)?rows-1-row:row;
            unsigned pattern=tall?(s[1]&1u)*0x1000u+(s[1]&0xFEu)*16u+(unsigned)(r/8)*16u:
                cyc_render_line_sprite_table(y)+(unsigned)s[1]*16u;
            unsigned lo=cyc_render_chr((uint16_t)(pattern+(r&7)));
            unsigned hi=cyc_render_chr((uint16_t)(pattern+(r&7)+8));
            for (int column=0;column<8;++column) {
                int bit=(s[2]&0x40)?column:7-column;
                uint8_t color=(uint8_t)(((lo>>bit)&1u)|(((hi>>bit)&1u)<<1));
                unsigned at=(unsigned)((y-top+row)*width+s[3]-left+column);
                if (color && !indices[at]) indices[at]=color;
            }
        }
    }
    arcade_colors_capture_preview(player,indices,width,height);
}

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
    cyc_render_set_sprite_owners(observe_sprite_owner,NULL);
}

void arcade_video_frame(void) {
    ++stats.frames;
    capture_player_preview(0); capture_player_preview(1);
    from.eligible = to.eligible = 0;
    from.color_enabled = to.color_enabled = 0;
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
    from.color_enabled = arcade_colors_current(from.player_colors);
    to.color_enabled = arcade_colors_current(to.player_colors);
    if (parts != 2 && parts != 3 && !(parts == 1 && from.color_enabled)) return NULL;
    const uint32_t *out = arcade_smooth_render_pair(&from, &to, part, parts, width, height);
    if (out) { current_scale = ARCADE_SMOOTH_SCALE; if (part) ++stats.rendered; }
    else {
        ++stats.native_fallback;
        /* Rejected interpolation must not discard character colors. The
         * native mask comes from the actual winning PPU sprite pixels, so
         * animation changes, death and other moving objects are independent. */
        if (from.color_enabled && arcade_recolor_native(colored_native,cyc_frame_argb(),
                cyc_render_sprite_pixels(),from.color_enabled,from.player_colors)) {
            *width=256; *height=240; return colored_native;
        }
    }
    return out;
}
void arcade_video_stats(ArcadeVideoStats *out) { if (out) *out = stats; }
