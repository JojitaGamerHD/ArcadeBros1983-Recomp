/* Read-only classification of the supported game's completed OAM upload.
 * Entity mode/state may change at any time after upload; provenance retains
 * the owner until that physical OAM entry is written again. */
#ifndef ARCADE_PLAYER_SPRITE_H
#define ARCADE_PLAYER_SPRITE_H
#include <stdint.h>
#include <string.h>

enum { ARCADE_SPRITE_ENTITIES=10, ARCADE_PREVIEW_MAX=32 };
typedef struct {
    unsigned gameplay, bonus_results;
    uint8_t active[10], state[2], draw[10], offset[10], shadow[256];
} ArcadeSpriteLayout;

/* The phase-end wait (9) retains the scene it follows. Bonus results (6)
 * draw their portraits directly into OAM, independently of entity RAM.
 * Source: RussianManSMWC/Mario-Bros.-Dis @ 0b7ca4de3cac44fb30b579e50691a809f1129816,
 * MarioBrosDisCA65.asm: GameplayCodePointers_C510, CODE_E333, CODE_E506,
 * CODE_E53D, WaitForGameplayModeChange_E129, LoadGame_D3F9 and
 * PlayerOAMTileData_CoinCountScreen_E574.
 * https://github.com/RussianManSMWC/Mario-Bros.-Dis/blob/0b7ca4de3cac44fb30b579e50691a809f1129816/MarioBrosDisCA65.asm
 */
static inline void arcade_sprite_scene(ArcadeSpriteLayout *layout, unsigned mode,
                                      unsigned next_mode, unsigned bonus_count,
                                      unsigned demo, unsigned complete) {
    /* 9 changes to 1 before the NMI which uploads the still-visible old OAM.
     * Only a completed real phase may use this loading-frame exception. */
    unsigned loading=mode==1 && !demo && complete;
    layout->gameplay=mode==4 || mode==9 || loading || (mode==6 && bonus_count==0);
    layout->bonus_results=mode==6 ||
        (((mode==9 && next_mode==1) || loading) && bonus_count==3);
    /* The pause/unpause routines retain the backed-up scene in $3B. */
    if (mode==5 || mode==10) {
        layout->gameplay=next_mode==4;
        layout->bonus_results=next_mode==6;
    }
}

static inline unsigned arcade_bonus_sprite_players(const ArcadeSpriteLayout *layout) {
    unsigned players=0;
    for (unsigned player=0;player<2;++player) {
        unsigned valid=1;
        for (unsigned tile=0;tile<6;++tile) {
            const uint8_t *s=layout->shadow+4*(player*6+tile);
            if (s[0]!=(player?0x78u:0x40u)+(tile/2)*8u ||
                s[1]!=0x12u+(tile^1u) || (s[2]&0xE3u)!=(0x40u|player) ||
                s[3]!=0x20u+(tile&1u)*8u) { valid=0; break; }
        }
        if (valid) players|=1u<<player;
    }
    return players;
}

static inline unsigned arcade_player_sprite_owner(const ArcadeSpriteLayout *layout,
                                                  unsigned slot, const uint8_t entry[4]) {
    static const unsigned counts[8]={6,4,2,2,2,2,2,1};
    if (!layout || (!layout->gameplay && !layout->bonus_results) || slot>=64 || entry[0]>=0xEF) return 0;
    for (unsigned b=0;b<4;++b) {
        unsigned mask=b==2?0xE3u:0xFFu;
        if ((entry[b]&mask)!=(layout->shadow[slot*4+b]&mask)) return 0;
    }
    if (layout->bonus_results) {
        unsigned players=arcade_bonus_sprite_players(layout);
        /* A complete six-tile portrait identifies this OAM layout. Do not
         * apply stale entity ranges to its coins, HUD or other sprites. */
        if (players) return slot<12 && (players&(1u<<(slot/6))) ? slot/6+1 : 0;
        if (!layout->gameplay) return 0;
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
