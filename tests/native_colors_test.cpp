/* Synthetic NES machine: exercise the real PPU's presentation-only sprite
 * provenance plus the game's upload-time ownership rules, without a ROM. */
extern "C" {
#include "cyc_core.h"
#include "cyc_render.h"
#include "hw_internal.h"
}
#include "player_sprite.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <memory>
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)

static ArcadeSpriteLayout layout;
static unsigned owner(unsigned slot,const uint8_t entry[4],void*) {
    return arcade_player_sprite_owner(&layout,slot,entry);
}
static void upload(unsigned slot,const uint8_t entry[4]) {
    ppu.show_bg=ppu.show_spr=0; ppu.scanline=241;
    ppu_write(0x2003,(uint8_t)(slot*4));
    for(unsigned i=0;i<4;++i) ppu_write(0x2004,entry[i]);
}
static void pixel_setup(bool behind,bool bg,bool clip) {
    ppu_power_on(); ppu.scanline=30; ppu.dot=0;
    ppu.show_spr=ppu.instant_spr=ppu.eval_spr=1;
    ppu.show_bg=ppu.instant_bg=ppu.eval_bg=bg?1:0;
    ppu.show_spr8=clip?0:1; ppu.show_bg8=1;
    ppu.spr_lo[0]=0xFF; ppu.spr_x[0]=0;
    ppu.spr_attr[0]=behind?0x20:0;
    ppu.bg_lo=0xFFFF; ppu.bg_hi=0; ppu.fine_x=0;
    cyc_sprite_capture.unit_owner[0]=1;
}
static void dots(unsigned count) {for(unsigned i=0;i<count;++i){ppu_dot();ppu_half_dot();}}
static void bonus_portraits(ArcadeSpriteLayout &s) {
    /* Independent fixture transcribed from E574 in the linked disassembly. */
    static const uint8_t portraits[48]={
        0x40,0x13,0x40,0x20, 0x40,0x12,0x40,0x28,
        0x48,0x15,0x40,0x20, 0x48,0x14,0x40,0x28,
        0x50,0x17,0x40,0x20, 0x50,0x16,0x40,0x28,
        0x78,0x13,0x41,0x20, 0x78,0x12,0x41,0x28,
        0x80,0x15,0x41,0x20, 0x80,0x14,0x41,0x28,
        0x88,0x17,0x41,0x20, 0x88,0x16,0x41,0x28
    };
    std::memcpy(s.shadow,portraits,sizeof portraits);
}

int main() {
    std::vector<uint8_t> rom(16+16384+8192,0);
    std::memcpy(rom.data(),"NES\x1A",4); rom[4]=1; rom[5]=1;
    CHECK(cyc_load_ines(rom.data(),rom.size())); cyc_power_on(0);
    cyc_render_set_sprite_owners(owner,nullptr);

    /* Phase-end waits use the unchanged entity OAM, even when a dying
     * player's active flag is clear. No interpolation is involved. */
    layout={}; arcade_sprite_scene(&layout,9,1,0,0,0);
    layout.state[0]=3; layout.draw[0]=2; layout.offset[0]=16;
    const uint8_t end_phase[4]={39,2,0,20}; std::memcpy(layout.shadow+16,end_phase,4);
    upload(4,end_phase); CHECK(cyc_render_sprite_owner(4)==1);
    arcade_sprite_scene(&layout,9,1,3,0,0); /* results pointer can remain stale on later phases */
    upload(4,end_phase); CHECK(cyc_render_sprite_owner(4)==1);
    arcade_sprite_scene(&layout,1,1,3,0,1); /* completed phase's final visible loading frame */
    upload(4,end_phase); CHECK(cyc_render_sprite_owner(4)==1);
    arcade_sprite_scene(&layout,1,1,3,0,0); /* initialization, not a completed phase */
    upload(4,end_phase); CHECK(cyc_render_sprite_owner(4)==0);
    arcade_sprite_scene(&layout,1,1,3,1,1); /* demo/title may have stale game flags */
    upload(4,end_phase); CHECK(cyc_render_sprite_owner(4)==0);
    arcade_sprite_scene(&layout,9,6,0,0,0); /* bonus play waiting for its results */
    upload(4,end_phase); CHECK(cyc_render_sprite_owner(4)==1);
    arcade_sprite_scene(&layout,6,6,0,0,0); /* first results frame before screen init */
    upload(4,end_phase); CHECK(cyc_render_sprite_owner(4)==1);

    /* Bonus results bypass entity RAM entirely. Both players can be inactive;
     * only their exact, complete six-tile portraits receive their colors. */
    layout={}; bonus_portraits(layout); arcade_sprite_scene(&layout,6,6,1,0,0);
    for(unsigned slot=0;slot<12;++slot) {
        upload(slot,layout.shadow+slot*4);
        CHECK(cyc_render_sprite_owner(slot)==slot/6+1);
    }
    const uint8_t coin[4]={0x41,0x30,2,0x70}; std::memcpy(layout.shadow+48,coin,4);
    layout.active[0]=1; layout.draw[0]=0; layout.offset[0]=48; /* stale entity range */
    upload(12,coin); CHECK(cyc_render_sprite_owner(12)==0);
    const uint8_t lookalike[4]={0x40,0x13,0x40,0x20}; std::memcpy(layout.shadow+200,lookalike,4);
    upload(50,lookalike); CHECK(cyc_render_sprite_owner(50)==0); /* no palette/RGB inference */
    uint8_t wrong[4]; std::memcpy(wrong,layout.shadow,4); ++wrong[1];
    upload(0,wrong); CHECK(cyc_render_sprite_owner(0)==0); /* actual OAM disagrees with shadow */
    ++layout.shadow[3]; /* incomplete/incorrect Mario table */
    upload(1,layout.shadow+4); CHECK(cyc_render_sprite_owner(1)==0);
    upload(6,layout.shadow+24); CHECK(cyc_render_sprite_owner(6)==2);
    --layout.shadow[3];
    for(unsigned mode: {5u,10u,9u}) {
        arcade_sprite_scene(&layout,mode,mode==9?1:6,3,0,0);
        upload(0,layout.shadow); CHECK(cyc_render_sprite_owner(0)==1);
        upload(11,layout.shadow+44); CHECK(cyc_render_sprite_owner(11)==2);
        upload(12,coin); CHECK(cyc_render_sprite_owner(12)==0);
    }
    arcade_sprite_scene(&layout,9,1,3,0,0);
    /* A complete results portrait blocks stale entity claims elsewhere. */
    upload(12,coin); CHECK(cyc_render_sprite_owner(12)==0);
    arcade_sprite_scene(&layout,1,1,3,0,1); /* final visible frame after bonus results */
    upload(0,layout.shadow); CHECK(cyc_render_sprite_owner(0)==1);
    upload(11,layout.shadow+44); CHECK(cyc_render_sprite_owner(11)==2);
    upload(12,coin); CHECK(cyc_render_sprite_owner(12)==0);
    arcade_sprite_scene(&layout,1,1,3,1,1);
    upload(0,layout.shadow); CHECK(cyc_render_sprite_owner(0)==0);
    arcade_sprite_scene(&layout,1,1,3,0,0);
    upload(11,layout.shadow+44); CHECK(cyc_render_sprite_owner(11)==0);
    for(unsigned mode: {0u,1u,2u,3u,7u,8u,11u}) {
        arcade_sprite_scene(&layout,mode,1,3,0,0);
        upload(0,layout.shadow); CHECK(cyc_render_sprite_owner(0)==0);
    }
    arcade_sprite_scene(&layout,6,6,1,0,0); /* a one-player results screen */
    std::memset(layout.shadow+24,0xEF,24);
    upload(0,layout.shadow); CHECK(cyc_render_sprite_owner(0)==1);
    upload(6,layout.shadow+24); CHECK(cyc_render_sprite_owner(6)==0);

    layout={}; layout.gameplay=1; layout.active[0]=1; layout.draw[0]=1; layout.offset[0]=8;
    const uint8_t body[4]={39,2,0,20}; std::memcpy(layout.shadow+8,body,4);
    upload(2,body); CHECK(cyc_render_sprite_owner(2)==1);
    /* Death and animation changes no longer require a stable frame pair. */
    layout.state[0]=3; layout.active[0]=0; layout.draw[0]=2;
    upload(2,body); CHECK(cyc_render_sprite_owner(2)==1);
    layout.offset[0]=20; std::memcpy(layout.shadow+20,body,4);
    upload(5,body); CHECK(cyc_render_sprite_owner(5)==1);
    upload(2,body); CHECK(cyc_render_sprite_owner(2)==0); /* old slot is not Mario */
    layout.active[2]=1; layout.draw[2]=2; layout.offset[2]=20;
    upload(5,body); CHECK(cyc_render_sprite_owner(5)==0); /* ambiguous/enemy range */
    layout.active[2]=0;
    uint8_t different[4]={39,7,0,20}; upload(5,different); CHECK(cyc_render_sprite_owner(5)==0);
    layout.gameplay=0; upload(5,body); CHECK(cyc_render_sprite_owner(5)==0);
    layout.gameplay=1; upload(5,body); CHECK(cyc_render_sprite_owner(5)==1);

    /* Actual evaluation carries the primary OAM label into secondary OAM and
     * into the hardware sprite units, without changing their contents. */
    ppu.scanline=40; ppu.dot=64; ppu.show_spr=ppu.instant_spr=ppu.eval_spr=1;
    ppu.oam_addr=20; ppu.oam2_addr=0; ppu.oam2_full=0; ppu.eval_tick=0;
    dots(256); CHECK(cyc_sprite_capture.unit_owner[0]==1);

    pixel_setup(false,false,false); dots(3);
    CHECK(cyc_sprite_capture.frame[30*256]==0); /* no early output */
    dots(1); CHECK(cyc_sprite_capture.frame[30*256]==5); /* owner1, pattern1, 3-dot delay */
    uint64_t observed=ppu_state_hash(1469598103934665603ull);
    uint16_t native_index=hw_frame_index[30*256];
    cyc_sprite_enabled=false; pixel_setup(false,false,false); dots(4);
    CHECK(ppu_state_hash(1469598103934665603ull)==observed);
    CHECK(hw_frame_index[30*256]==native_index); /* logic and original RGB path unchanged */
    cyc_sprite_enabled=true;
    pixel_setup(true,true,false); dots(4); CHECK(cyc_sprite_capture.frame[30*256]==0); /* BG hides sprite */
    pixel_setup(false,true,false); dots(4); CHECK(cyc_sprite_capture.frame[30*256]==5);
    pixel_setup(false,false,true); dots(4); CHECK(cyc_sprite_capture.frame[30*256]==0); /* left clip */
    pixel_setup(false,false,false); cyc_sprite_capture.unit_owner[0]=0; dots(4);
    CHECK(cyc_sprite_capture.frame[30*256]==0); /* unlabelled sprite, same palette */
    pixel_setup(false,false,false); ppu.spr_attr[0]=1; cyc_sprite_capture.unit_owner[0]=2; dots(4);
    CHECK(cyc_sprite_capture.frame[30*256]==9); /* Luigi's identity is not RGB/palette lookup */

    auto original=std::make_unique<uint32_t[]>(256*240);
    auto recolored=std::make_unique<uint32_t[]>(256*240);
    for(unsigned i=0;i<256*240;++i) original[i]=0xFF334455;
    uint32_t rgb[2][3]={{0xFF123456,0xFF123457,0xFF123458},{0xFFABCDEF,0xFFABCDEE,0xFFABCDED}};
    CHECK(arcade_recolor_native(recolored.get(),original.get(),cyc_render_sprite_pixels(),3,rgb)>0);
    CHECK(recolored[30*256]==rgb[1][0] && recolored[0]==original[0]);
    CHECK(arcade_recolor_native(recolored.get(),original.get(),cyc_render_sprite_pixels(),1,rgb)==0);
    CHECK(recolored[30*256]==original[30*256]);
    ppu_state_reloaded(); CHECK(cyc_render_sprite_owner(5)==0 && cyc_sprite_capture.frame[30*256]==0);
    std::puts("PASS native sprite colors: phase-end waits, exact bonus portraits and transitions, upload ownership, death, reorder, actual PPU evaluation/pipeline, BG priority, clipping and inert state hash.");
    return 0;
}
