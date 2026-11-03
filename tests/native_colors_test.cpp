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

int main() {
    std::vector<uint8_t> rom(16+16384+8192,0);
    std::memcpy(rom.data(),"NES\x1A",4); rom[4]=1; rom[5]=1;
    CHECK(cyc_load_ines(rom.data(),rom.size())); cyc_power_on(0);
    cyc_render_set_sprite_owners(owner,nullptr);
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
    std::puts("PASS native sprite colors: upload ownership, death, reorder, actual PPU evaluation/pipeline, BG priority, clipping and inert state hash.");
    return 0;
}
