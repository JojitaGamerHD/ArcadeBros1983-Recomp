/* Presentation preferences only. No CPU/PPU writes or module ABI changes. */
#include "player_colors.h"
#include <string.h>
#include <stdlib.h>

enum { SLOTS = 8 };
static uint32_t active[2][3] = {{0xF83800,0xFFB8B0,0x0078F8},{0x00B800,0xFFB8B0,0xFCFCFC}};
static uint32_t presets[SLOTS][2][3];
static unsigned enabled, valid;
static int selected;
static RecompRuntimeUiColorPreview preview[2];

static int hex6(const char *s, uint32_t *out) {
    if (*s == '#') ++s;
    if (strlen(s) != 6) return 0;
    uint32_t v = 0;
    for (int i = 0; i < 6; ++i) {
        int d = s[i] >= '0' && s[i] <= '9' ? s[i]-'0' :
                s[i] >= 'a' && s[i] <= 'f' ? s[i]-'a'+10 :
                s[i] >= 'A' && s[i] <= 'F' ? s[i]-'A'+10 : -1;
        if (d < 0) return 0;
        v = (v << 4) | (unsigned)d;
    }
    *out = v; return 1;
}
static int color_index(const char *key) {
    static const char *keys[] = {"arcade.color.m1","arcade.color.m2","arcade.color.m3",
                                "arcade.color.l1","arcade.color.l2","arcade.color.l3"};
    for (int i = 0; i < 6; ++i) if (!strcmp(key, keys[i])) return i;
    return -1;
}
unsigned arcade_colors_current(uint32_t out[2][3]) {
    for (int p = 0; p < 2; ++p) for (int c = 0; c < 3; ++c)
        out[p][c] = 0xFF000000u | active[p][c];
    return enabled ? 3 : 0;
}
int arcade_colors_get(void *ctx, const RecompRuntimeUiItem *it, int *v) {
    (void)ctx;
    int color = color_index(it->key);
    if (color >= 0) *v = (int)active[color/3][color%3];
    else if (!strcmp(it->key,"arcade.color.enabled")) *v = (int)enabled;
    else if (!strcmp(it->key,"arcade.color.slot")) *v = selected + 1;
    else return 0;
    return 1;
}
int arcade_colors_set(void *ctx, const RecompRuntimeUiItem *it, int v) {
    (void)ctx;
    int color = color_index(it->key);
    if (color >= 0 && v >= 0 && v <= 0xFFFFFF) active[color/3][color%3] = (uint32_t)v;
    else if (!strcmp(it->key,"arcade.color.enabled") && (v == 0 || v == 1)) enabled = (unsigned)v;
    else if (!strcmp(it->key,"arcade.color.slot") && v >= 1 && v <= SLOTS) selected = v - 1;
    else return 0;
    return 1;
}

void arcade_colors_capture_preview(unsigned player, const uint8_t *indices, int width, int height) {
    if (player >= 2 || !indices || width < 1 || height < 1 ||
        width > RECOMP_RUNTIME_UI_PREVIEW_MAX || height > RECOMP_RUNTIME_UI_PREVIEW_MAX) return;
    int visible = 0;
    for (int i = 0; i < width * height; ++i) {
        if (indices[i] > 3) return;
        visible |= indices[i];
    }
    if (!visible) return;
    preview[player].width = width;
    preview[player].height = height;
    memcpy(preview[player].pixels,indices,(size_t)(width * height));
}
int arcade_colors_preview(void *ctx, const RecompRuntimeUiItem *it, RecompRuntimeUiColorPreview *out) {
    (void)ctx;
    int color = color_index(it->key);
    if (color < 0 || !out || !preview[color/3].width) return 0;
    *out = preview[color/3];
    for (int i = 0; i < 3; ++i) out->palette[i] = 0xFF000000u | active[color/3][i];
    out->selected_index = color%3;
    return 1;
}
int arcade_colors_get_text(void *ctx, const RecompRuntimeUiItem *it, char *out, size_t n) {
    (void)ctx; int i = color_index(it->key);
    if (i < 0) return 0;
    snprintf(out,n,"#%06X",(unsigned)active[i/3][i%3]); return 1;
}
int arcade_colors_set_text(void *ctx, const RecompRuntimeUiItem *it, const char *v) {
    (void)ctx; int i = color_index(it->key); uint32_t rgb;
    if (i < 0 || !hex6(v,&rgb)) return 0;
    active[i/3][i%3] = rgb; return 1;
}
int arcade_colors_action(void *ctx, const RecompRuntimeUiItem *it) {
    (void)ctx;
    if (!strcmp(it->key,"arcade.color.store")) {
        memcpy(presets[selected],active,sizeof active); valid |= 1u << selected;
    } else if (!strcmp(it->key,"arcade.color.load")) {
        if (!(valid & (1u << selected))) return 0;
        memcpy(active,presets[selected],sizeof active); enabled = 1;
    } else if (!strcmp(it->key,"arcade.color.reset")) enabled = 0;
    else return 0;
    return 1;
}
void arcade_colors_load(void *ctx, const char *key, const char *value) {
    (void)ctx;
    if (!strcmp(key,"ColorsEnabled")) { if (!strcmp(value,"0") || !strcmp(value,"1")) enabled = (unsigned)(value[0]-'0'); return; }
    if (!strcmp(key,"ColorsSlot")) { if (strlen(value)==1 && value[0]>='1' && value[0]<='8') selected=value[0]-'1'; return; }
    if (!strcmp(key,"ColorsActive")) {
        uint32_t tmp[2][3];
        if (strlen(value) != 41) return;
        for (int i=0;i<6;++i) {
            char rgb[7]; memcpy(rgb,value+i*7,6); rgb[6]=0;
            if (!hex6(rgb,&tmp[i/3][i%3]) || (i<5 && value[i*7+6]!=',')) return;
        }
        memcpy(active,tmp,sizeof active); return;
    }
    for (int slot=0;slot<SLOTS;++slot) {
        char name[32]; snprintf(name,sizeof name,"ColorsPreset%d",slot+1);
        if (strcmp(key,name)) continue;
        uint32_t tmp[2][3];
        if (strlen(value)!=41) return;
        for (int i=0;i<6;++i) {
            char rgb[7]; memcpy(rgb,value+i*7,6); rgb[6]=0;
            if (!hex6(rgb,&tmp[i/3][i%3]) || (i<5 && value[i*7+6]!=',')) return;
        }
        memcpy(presets[slot],tmp,sizeof tmp); valid |= 1u << slot; return;
    }
}
static void write_rgb(FILE *f, const uint32_t rgb[2][3]) {
    for (int i=0;i<6;++i) fprintf(f,"%s%06X",i?",":"",(unsigned)rgb[i/3][i%3]);
    fputc('\n',f);
}
void arcade_colors_save(void *ctx, FILE *f) {
    (void)ctx;
    fprintf(f,"ColorsEnabled = %u\nColorsSlot = %d\nColorsActive = ",enabled,selected+1);
    write_rgb(f,active);
    for (int i=0;i<SLOTS;++i) if (valid & (1u << i)) {
        fprintf(f,"ColorsPreset%d = ",i+1); write_rgb(f,presets[i]);
    }
}
