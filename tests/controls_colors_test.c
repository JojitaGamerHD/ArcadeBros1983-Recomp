#include "cyc_settings.h"
#include "player_colors.h"
#include <SDL.h>
#include <string.h>
#include <stdio.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(int argc, char **argv) {
    (void)argv;
    CycSettings s, loaded; CycInputState out;
    uint8_t keys[SDL_NUM_SCANCODES]={0};
    CycInputFrame f={0}; f.keys=keys; f.key_count=SDL_NUM_SCANCODES; f.pad_count=2;
    f.pad_guid[0]="first"; f.pad_guid[1]="second";
    cyc_settings_default(&s);
    s.bind.button[0][0].key=SDL_SCANCODE_C;
    s.bind.button[0][0].pad=CYC_PAD_BUTTON(SDL_CONTROLLER_BUTTON_Y);
    s.bind.button[1][0].pad=CYC_PAD_AXIS(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,1);
    strcpy(s.bind.device[1],"second");
    s.bind.shortcut[CYC_SC_MENU].key=SDL_SCANCODE_F10;
    /* Game-owned extras share the real host settings serializer. */
    CycSettingsGame game={0,arcade_colors_load,arcade_colors_save};
    RecompRuntimeUiItem color={0}, enabled={0}, slot={0}, store={0}, load={0}, reset={0};
    color.key="arcade.color.m1"; enabled.key="arcade.color.enabled";
    slot.key="arcade.color.slot"; store.key="arcade.color.store";
    load.key="arcade.color.load"; reset.key="arcade.color.reset";
    if (argc > 1) {
        uint32_t rgb[2][3];
        CHECK(arcade_colors_current(rgb)==0);
        CHECK(cyc_settings_load(&loaded,"controls-colors-test.ini",stderr,&game));
        CHECK(loaded.bind.button[0][0].key==SDL_SCANCODE_C);
        CHECK(arcade_colors_current(rgb)==3 && rgb[0][0]==0xFF123ABCu);
        CHECK(arcade_colors_set_text(0,&color,"FFFFFF"));
        CHECK(arcade_colors_action(0,&load));
        CHECK(arcade_colors_current(rgb)==3 && rgb[0][0]==0xFF123ABCu);
        remove("controls-colors-test.ini");
        puts("PASS new process reloads controls, active RGB, selected slot and saved preset.");
        return 0;
    }
    CHECK(arcade_colors_set_text(0,&color,"#123ABC"));
    CHECK(!arcade_colors_set_text(0,&color,"12345G"));
    CHECK(!arcade_colors_set_text(0,&color,"#123ABC00"));
    CHECK(arcade_colors_set(0,&slot,8)); CHECK(!arcade_colors_set(0,&slot,9));
    CHECK(!arcade_colors_action(0,&load)); /* empty preset */
    CHECK(arcade_colors_action(0,&store));
    CHECK(arcade_colors_set(0,&enabled,1));
    CHECK(cyc_settings_save(&s,"controls-colors-test.ini",&game));
    CHECK(arcade_colors_set_text(0,&color,"FFFFFF"));
    CHECK(arcade_colors_action(0,&reset));
    cyc_settings_default(&loaded);
    CHECK(cyc_settings_load(&loaded,"controls-colors-test.ini",stderr,&game));
    CHECK(loaded.bind.button[0][0].key==SDL_SCANCODE_C);
    CHECK(loaded.bind.button[0][0].pad==CYC_PAD_BUTTON(SDL_CONTROLLER_BUTTON_Y));
    CHECK(loaded.bind.button[1][0].pad==CYC_PAD_AXIS(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,1));
    CHECK(!strcmp(loaded.bind.device[1],"second"));
    keys[SDL_SCANCODE_Z]=1; cyc_input_eval(&loaded.bind,&f,&out);
    CHECK(!(out.buttons[0]&0x80)); /* former binding no longer plays */
    keys[SDL_SCANCODE_C]=1; cyc_input_eval(&loaded.bind,&f,&out); CHECK(out.buttons[0]&0x80);
    memset(keys,0,sizeof keys);
    f.pad_buttons[0]=1u<<SDL_CONTROLLER_BUTTON_Y;
    f.pad_axes[1][SDL_CONTROLLER_AXIS_TRIGGERRIGHT]=26000;
    cyc_input_eval(&loaded.bind,&f,&out); CHECK((out.buttons[0]&0x80) && (out.buttons[1]&0x80));
    CHECK(out.player_pad[0]==0 && out.player_pad[1]==1);
    loaded.bind.button[0][0].pad=CYC_PAD_AXIS(SDL_CONTROLLER_AXIS_LEFTX,1);
    f.pad_buttons[0]=0; f.pad_axes[0][SDL_CONTROLLER_AXIS_LEFTX]=26000;
    cyc_input_eval(&loaded.bind,&f,&out); CHECK(out.buttons[0]==0x80); /* no implicit Right */
    f.pad_axes[0][SDL_CONTROLLER_AXIS_LEFTX]=0;
    f.pad_buttons[0]=0; f.pad_axes[1][SDL_CONTROLLER_AXIS_TRIGGERRIGHT]=0;
    keys[SDL_SCANCODE_F10]=1; cyc_input_eval(&loaded.bind,&f,&out); CHECK(out.shortcut[CYC_SC_MENU]);
    memset(keys,0,sizeof keys); loaded.bind.button[0][0].key=SDL_SCANCODE_TAB;
    keys[SDL_SCANCODE_TAB]=1; cyc_input_eval(&loaded.bind,&f,&out); CHECK(out.buttons[0]&0x80);
    loaded.bind.button[0][0].key=SDL_SCANCODE_F10; keys[SDL_SCANCODE_F10]=1;
    cyc_input_eval(&loaded.bind,&f,&out); CHECK(!(out.buttons[0]&0x80)); /* hand-edited shortcut still wins */
    uint32_t rgb[2][3]; CHECK(arcade_colors_current(rgb)==3 && rgb[0][0]==0xFF123ABCu);
    CHECK(arcade_colors_set_text(0,&color,"FFFFFF")); CHECK(arcade_colors_action(0,&load));
    CHECK(arcade_colors_current(rgb)==3 && rgb[0][0]==0xFF123ABCu);
    arcade_colors_load(0,"ColorsActive","invalid");
    CHECK(arcade_colors_current(rgb)==3 && rgb[0][0]==0xFF123ABCu);
    CHECK(arcade_colors_action(0,&reset)); CHECK(arcade_colors_current(rgb)==0);
    cyc_bindings_default(&loaded.bind); CHECK(loaded.bind.button[0][0].key==SDL_SCANCODE_Z);
    puts("PASS persisted controls, keyboard/pads/axes, player routing, shortcuts, RGB validation and presets.");
    return 0;
}
