#include "smooth_video.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); return 1; } } while (0)
constexpr uint32_t BG = 0xff102030u;
constexpr uint32_t RED = 0xffef3020u;
constexpr uint32_t GREEN = 0xff30ed20u;
constexpr int SW = 256 * 6;
constexpr int SH = 240 * 6;

void blank(ArcadeSmoothFrame &f) {
    std::memset(&f, 0, sizeof(f));
    std::fill_n(f.background, ARCADE_SMOOTH_PIXELS, BG);
    std::fill_n(f.native, ARCADE_SMOOTH_PIXELS, BG);
    std::fill_n(f.oam, 256, uint8_t(255));
    std::fill_n(f.mask, 240, uint8_t(0x1e));
    f.palette[17] = RED;
    f.palette[18] = GREEN;
    f.palette[21] = GREEN;
    f.scene = 1;
    f.eligible = 1;
}
void sprite(ArcadeSmoothFrame &f, int slot, int x, int y, int group = 0, uint8_t attr = 0) {
    f.oam[slot * 4] = uint8_t(y - 1);
    f.oam[slot * 4 + 1] = 0;
    f.oam[slot * 4 + 2] = attr;
    f.oam[slot * 4 + 3] = uint8_t(x);
    f.group[slot] = uint8_t(group);
    if (group) {
        f.group_active[group] = 1;
        f.group_x[group] = int16_t(x);
        f.group_y[group] = int16_t(y);
    }
    std::fill_n(f.chr, 8, uint8_t(255));
}
uint64_t hash(const uint32_t *p) {
    uint64_t value = 1469598103934665603ull;
    for (int i = 0; i < SW * SH; ++i) value = (value ^ p[i]) * 1099511628211ull;
    return value;
}
size_t count(const uint32_t *p, uint32_t color) {
    return size_t(std::count(p, p + SW * SH, color));
}

int main() {
    auto a = std::make_unique<ArcadeSmoothFrame>();
    auto b = std::make_unique<ArcadeSmoothFrame>();
    int w, h;
    blank(*a); sprite(*a,0,30,40,1); *b = *a;
    b->group_x[1] = 34; b->oam[3] = 34;
    auto p = arcade_smooth_render_pair(a.get(),b.get(),0,1,&w,&h);
    CHECK(p && w == SW && h == SH);
    CHECK(count(p,RED) == 64u*36u);
    const auto h0 = hash(p);
    p = arcade_smooth_render_pair(a.get(),b.get(),1,3,&w,&h);
    CHECK(p && p[40*6*SW + 188] == RED && p[40*6*SW + 187] == BG);
    const auto h1 = hash(p);
    p = arcade_smooth_render_pair(a.get(),b.get(),1,2,&w,&h);
    CHECK(p && p[40*6*SW + 192] == RED && p[40*6*SW + 191] == BG);
    CHECK(count(p,RED) == 64u*36u); // no old image or translucent duplicate
    const auto h2 = hash(p);
    p = arcade_smooth_render_pair(a.get(),b.get(),2,3,&w,&h);
    CHECK(p && p[40*6*SW + 196] == RED);
    const auto h3 = hash(p);
    CHECK(h0 != h1 && h0 != h2 && h0 != h3 && h1 != h2 && h1 != h3 && h2 != h3);

    // A composite entity keeps its tile spacing when OAM slots are reassigned.
    sprite(*a,1,38,40,1); a->group_x[1] = 30;
    *b = *a; b->group_x[1] = 34;
    b->oam[3] = 42; b->oam[7] = 34; // slot order changed, identity did not
    p = arcade_smooth_render_pair(a.get(),b.get(),1,2,&w,&h);
    CHECK(p && count(p,RED) == 128u*36u);
    CHECK(p[40*6*SW+192] == RED && p[40*6*SW+239] == RED && p[40*6*SW+240] == RED);

    blank(*a); sprite(*a,0,254,40,1); *b = *a;
    b->group_x[1] = 2; b->oam[3] = 2;
    p = arcade_smooth_render_pair(a.get(),b.get(),1,2,&w,&h);
    CHECK(p && p[40*6*SW] == RED && p[40*6*SW+128*6] == BG);
    b->group_x[1] = 40;
    CHECK(!arcade_smooth_render_pair(a.get(),b.get(),1,2,&w,&h));
    *b = *a; b->scene = 2;
    CHECK(!arcade_smooth_render_pair(a.get(),b.get(),1,2,&w,&h));
    *b = *a; b->oam[0] = 255;
    CHECK(!arcade_smooth_render_pair(a.get(),b.get(),1,2,&w,&h));
    *b = *a; b->group_active[1] = 0;
    CHECK(!arcade_smooth_render_pair(a.get(),b.get(),1,2,&w,&h));
    CHECK(!arcade_smooth_render_pair(a.get(),a.get(),1,5,&w,&h));

    // Priority arbitration must hide a later front sprite when the earlier
    // sprite is behind the opaque background at the same location.
    blank(*a); sprite(*a,0,20,40,0,0x20); sprite(*a,1,20,40,0,1);
    a->bg_opaque[40*256+20] = 1; *b = *a;
    p = arcade_smooth_render_pair(a.get(),b.get(),1,2,&w,&h);
    CHECK(p && p[40*6*SW+20*6] == BG && p[40*6*SW+21*6] == RED);
    CHECK(count(p,GREEN) == 0);

    blank(*a);
    for (int i = 0; i < 9; ++i) sprite(*a,i,20+i*10,40);
    *b = *a; p = arcade_smooth_render_pair(a.get(),b.get(),1,2,&w,&h);
    CHECK(p && count(p,RED) == 8u*64u*36u);
    CHECK(p[40*6*SW+100*6] == BG);

    blank(*a); sprite(*a,0,0,40); std::fill_n(a->mask,240,uint8_t(0x18));
    *b = *a; p = arcade_smooth_render_pair(a.get(),b.get(),1,2,&w,&h);
    CHECK(p && count(p,RED) == 0);
    std::fill_n(a->mask,240,uint8_t(0x1c)); *b = *a;
    p = arcade_smooth_render_pair(a.get(),b.get(),1,2,&w,&h);
    CHECK(p && count(p,RED) == 64u*36u);

    blank(*a); sprite(*a,0,40,40); std::fill_n(a->chr,8192,uint8_t(0));
    a->chr[0] = 0x80; a->oam[2] = 0xc0; *b = *a;
    p = arcade_smooth_render_pair(a.get(),b.get(),1,2,&w,&h);
    CHECK(p && count(p,RED) == 36 && p[47*6*SW+47*6] == RED);

    blank(*a); sprite(*a,0,40,40); std::fill_n(a->chr,8192,uint8_t(0));
    std::fill_n(a->sprite16,240,uint8_t(1)); a->oam[1] = 1;
    a->chr[0x1000] = 0x80;
    a->chr[0x1010+8] = 0x80;
    *b = *a; p = arcade_smooth_render_pair(a.get(),b.get(),1,2,&w,&h);
    CHECK(p && p[40*6*SW+40*6] == RED && p[48*6*SW+40*6] == GREEN);

    // Exact source at alpha zero can use the real hardware frame directly.
    a->native_valid = 1; a->native[0] = GREEN; *b = *a;
    p = arcade_smooth_render_pair(a.get(),b.get(),0,1,&w,&h);
    CHECK(p && p[0] == GREEN && p[SW*6+6] == BG && count(p,GREEN) == 36);
    std::puts("PASS geometry, thirds/halves, composite identity, wrapping, discontinuities, OAM/BG priority, eight-sprite limit, clipping and flips.");
    return 0;
}
