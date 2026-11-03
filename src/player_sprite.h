/* Read-only classification of the supported game's completed OAM upload.
 * Entity mode/state may change at any time after upload; provenance retains
 * the owner until that physical OAM entry is written again. */
#ifndef ARCADE_PLAYER_SPRITE_H
#define ARCADE_PLAYER_SPRITE_H
#include <stdint.h>
#include <string.h>

enum { ARCADE_SPRITE_ENTITIES=10, ARCADE_PREVIEW_MAX=32 };
typedef struct {
    unsigned gameplay;
    uint8_t active[10], state[2], draw[10], offset[10], shadow[256];
} ArcadeSpriteLayout;

static inline unsigned arcade_player_sprite_owner(const ArcadeSpriteLayout *layout,
                                                  unsigned slot, const uint8_t entry[4]) {
    static const unsigned counts[8]={6,4,2,2,2,2,2,1};
    if (!layout || !layout->gameplay || slot>=64 || entry[0]>=0xEF) return 0;
    for (unsigned b=0;b<4;++b) {
        unsigned mask=b==2?0xE3u:0xFFu;
        if ((entry[b]&mask)!=(layout->shadow[slot*4+b]&mask)) return 0;
    }
    unsigned owner=0, claims=0;
    for (unsigned e=0;e<ARCADE_SPRITE_ENTITIES;++e) {
        /* A dying player can have a nonzero state with its active flag clear.
         * It still owns its uploaded death-animation tiles. */
        if (!layout->active[e] && !(e<2 && layout->state[e])) continue;
        if (layout->draw[e]>=8 || (layout->offset[e]&3)) continue;
        unsigned first=layout->offset[e]/4u, count=counts[layout->draw[e]];
        if (first+count>64 || slot<first || slot>=first+count) continue;
        ++claims; owner=e<2?e+1:0;
    }
    return claims==1?owner:0;
}

static inline unsigned arcade_recolor_native(uint32_t *out, const uint32_t *native,
                                             const uint8_t *provenance, unsigned enabled,
                                             const uint32_t colors[2][3]) {
    unsigned changed=0;
    memcpy(out,native,256u*240u*sizeof(*out));
    if (!provenance) return 0;
    for (unsigned i=0;i<256u*240u;++i) {
        unsigned owner=provenance[i]>>2, color=provenance[i]&3u;
        if (owner<1 || owner>2 || !color || !(enabled&(1u<<(owner-1)))) continue;
        out[i]=colors[owner-1][color-1]; ++changed;
    }
    return changed;
}
#endif
