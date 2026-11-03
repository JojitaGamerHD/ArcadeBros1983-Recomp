/* Exercise the shipping RGB editor and the host's event arbitration without
 * a window, ROM or physical input device. */
#include "cyc_ui_text_input.h"
#include "player_colors.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "../recomp-ui/src/common/backends/imgui/runtime_ui_imgui.cpp"
#include <cstdio>
#include <cstring>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); return 1; } } while(0)

static void frame(RecompRuntimeUi *ui) {
    ImGui::NewFrame();
    recomp_runtime_ui_render_imgui(ui);
    ImGui::Render();
}

static SDL_Event key_event(SDL_Keycode key, SDL_Scancode scan) {
    SDL_Event event{};
    event.type=SDL_KEYDOWN; event.key.keysym.sym=key; event.key.keysym.scancode=scan;
    return event;
}

static int saves;
static void saved(void *) { ++saves; }

static int wheel_editor() {
    const RecompRuntimeUiItem items[]={
        {"arcade.color.m1","Colores","Mario - tono 1","Rueda de color",RECOMP_RUNTIME_UI_COLOR,0,0,0,nullptr,0,nullptr},
        {"arcade.color.l1","Colores","Luigi - tono 1","Rueda de color",RECOMP_RUNTIME_UI_COLOR,0,0,0,nullptr,0,nullptr}
    };
    /* Synthetic local capture for the test only. Production never embeds ROM
     * graphics and gets these indexed pixels from the running game's OAM/CHR. */
    const uint8_t pose[]={0,1,0,2,3,2};
    arcade_colors_capture_preview(0,pose,3,2);
    const uint8_t invalid[]={4}; arcade_colors_capture_preview(0,invalid,1,1);
    RecompRuntimeUiColorPreview preview{};
    CHECK(arcade_colors_preview(nullptr,&items[0],&preview));
    CHECK(preview.width==3 && preview.height==2 && preview.pixels[4]==3);
    CHECK(!arcade_colors_preview(nullptr,&items[1],&preview));
    RecompRuntimeUiConfig config{};
    config.title="Colores"; config.theme="nes"; config.items=items; config.item_count=2;
    config.callbacks.get_value=arcade_colors_get;
    config.callbacks.set_value=arcade_colors_set;
    config.callbacks.get_color_preview=arcade_colors_preview;
    config.callbacks.save=saved;
    RecompRuntimeUi *ui=recomp_runtime_ui_create(&config);
    CHECK(ui && recomp_runtime_ui_open_section(ui,"Colores"));
    ImGuiIO &io=ImGui::GetIO();
    io.DisplaySize=ImVec2(800,600); frame(ui);
    int original=0; CHECK(arcade_colors_get(nullptr,&items[0],&original));
    io.AddKeyEvent(ImGuiKey_Enter,true);
    CHECK(recomp_runtime_ui_handle_input(ui,RECOMP_RUNTIME_UI_INPUT_ACCEPT,1,0));
    frame(ui);
    CHECK(ui->editing_color && !ui->editing_text && recomp_runtime_ui_wants_text_input(ui));
    io.AddKeyEvent(ImGuiKey_Enter,false); frame(ui); frame(ui);
    ImGuiWindow *popup=ImGui::FindWindowByName("Elegir color##recomp-runtime-color");
    CHECK(popup && popup->Active && popup->Pos.x>=0 && popup->Pos.y>=0);
    CHECK(popup->Pos.x+popup->Size.x<=800 && popup->Pos.y+popup->Size.y<=600);
    CHECK(popup->ContentSize.y<=popup->Size.y-popup->TitleBarHeight-ImGui::GetStyle().WindowPadding.y*2);

    /* Click the actual hue wheel/triangle: the test is independent of the
     * font's precise line height, so try points inside its left-hand region. */
    int chosen=original;
    for (int y=100;y<=300 && chosen==original;y+=25)
        for (int x=30;x<=260 && chosen==original;x+=25) {
            io.AddMousePosEvent(popup->Pos.x+float(x),popup->Pos.y+float(y));
            io.AddMouseButtonEvent(0,true); frame(ui);
            io.AddMouseButtonEvent(0,false); frame(ui);
            CHECK(ui->editing_color);
            CHECK(arcade_colors_get(nullptr,&items[0],&chosen));
        }
    CHECK(chosen!=original && saves>0);
    CHECK(arcade_colors_preview(nullptr,&items[0],&preview));
    CHECK(preview.palette[0]==(0xFF000000u|unsigned(chosen)));
    CHECK(preview.pixels[4]==3 && preview.pixels[1]==1); // only color changes
    io.AddKeyEvent(ImGuiKey_Escape,true); frame(ui);
    CHECK(!ui->editing_color && recomp_runtime_ui_is_open(ui));
    int restored=0; CHECK(arcade_colors_get(nullptr,&items[0],&restored) && restored==original);
    io.AddKeyEvent(ImGuiKey_Escape,false); frame(ui);

    io.DisplaySize=ImVec2(640,480);
    CHECK(recomp_runtime_ui_handle_input(ui,RECOMP_RUNTIME_UI_INPUT_ACCEPT,1,0));
    frame(ui); frame(ui); frame(ui);
    popup=ImGui::FindWindowByName("Elegir color##recomp-runtime-color");
    CHECK(popup && popup->Pos.x>=0 && popup->Pos.y>=0);
    CHECK(popup->Pos.x+popup->Size.x<=640 && popup->Pos.y+popup->Size.y<=480);
    CHECK(popup->ContentSize.y<=popup->Size.y-popup->TitleBarHeight-ImGui::GetStyle().WindowPadding.y*2);
    CHECK(!recomp_runtime_ui_commit_color(ui,&items[0],-1));
    CHECK(!recomp_runtime_ui_commit_color(ui,&items[0],0x1000000));
    CHECK(recomp_runtime_ui_commit_color(ui,&items[0],0x2288EE));
    io.AddKeyEvent(ImGuiKey_Enter,true); frame(ui);
    CHECK(!ui->editing_color && recomp_runtime_ui_is_open(ui));
    CHECK(arcade_colors_get(nullptr,&items[0],&chosen) && chosen==0x2288EE);
    io.AddKeyEvent(ImGuiKey_Enter,false); frame(ui);
    recomp_runtime_ui_destroy(ui);
    return 0;
}

int main() {
    const RecompRuntimeUiItem color={"arcade.color.m1","Colores","Mario - color 1",
        "#RRGGBB",RECOMP_RUNTIME_UI_TEXT,0,0,0,nullptr,0,nullptr};
    RecompRuntimeUiConfig config{};
    config.title="Colores"; config.theme="nes";
    config.items=&color; config.item_count=1;
    config.callbacks.get_text=arcade_colors_get_text;
    config.callbacks.set_text=arcade_colors_set_text;
    RecompRuntimeUi *ui=recomp_runtime_ui_create(&config);
    CHECK(ui && recomp_runtime_ui_open_section(ui,"Colores"));
    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ImGuiIO &io=ImGui::GetIO(); io.IniFilename=nullptr;
    io.DisplaySize=ImVec2(1200,1000); io.DeltaTime=1.0f/60.0f;
    io.ConfigInputTrickleEventQueue=false;
    unsigned char *pixels=nullptr; int width=0,height=0;
    io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    frame(ui);
    SDL_Event opening=key_event(SDLK_RETURN,SDL_SCANCODE_RETURN);
    CHECK(!cyc_ui_text_owns_event(false,&opening));
    io.AddKeyEvent(ImGuiKey_Enter,true);
    CHECK(recomp_runtime_ui_handle_input(ui,RECOMP_RUNTIME_UI_INPUT_ACCEPT,1,0));
    frame(ui); io.AddKeyEvent(ImGuiKey_Enter,false); frame(ui);
    CHECK(recomp_runtime_ui_wants_text_input(ui));
    io.AddInputCharactersUTF8("#123ABC"); frame(ui);
    CHECK(!std::strcmp(ui->edit_buffer,"#123ABC"));

    /* Backspace belongs to InputText, not the menu's Back action. */
    SDL_Event event=key_event(SDLK_BACKSPACE,SDL_SCANCODE_BACKSPACE);
    CHECK(cyc_ui_text_owns_event(recomp_runtime_ui_wants_text_input(ui)!=0,&event));
    io.AddKeyEvent(ImGuiKey_Backspace,true); frame(ui);
    CHECK(ui->in_section && !std::strcmp(ui->edit_buffer,"#123AB"));
    io.AddKeyEvent(ImGuiKey_Backspace,false); frame(ui);
    io.AddInputCharactersUTF8("C"); frame(ui);
    CHECK(!std::strcmp(ui->edit_buffer,"#123ABC"));
    event=key_event(SDLK_LEFT,SDL_SCANCODE_LEFT);
    CHECK(cyc_ui_text_owns_event(true,&event));
    io.AddKeyEvent(ImGuiKey_LeftArrow,true); frame(ui);
    io.AddKeyEvent(ImGuiKey_LeftArrow,false); frame(ui);
    io.AddKeyEvent(ImGuiKey_RightArrow,true); frame(ui);
    io.AddKeyEvent(ImGuiKey_RightArrow,false); frame(ui);
    CHECK(ui->in_section && ui->row_index==0);

    /* Enter commits the actual ImGui buffer instead of resetting the editor
     * through menu navigation. Arbitrary host shortcuts stay suspended. */
    bool wait_release=false; CycInputState state{};
    state.shortcut[CYC_SC_MENU]=true; state.shortcut[CYC_SC_FULLSCREEN]=true;
    cyc_ui_text_filter_shortcuts(true,&wait_release,&state);
    CHECK(wait_release && !state.shortcut[CYC_SC_MENU] && !state.shortcut[CYC_SC_FULLSCREEN]);
    event=key_event(SDLK_RETURN,SDL_SCANCODE_RETURN);
    CHECK(cyc_ui_text_owns_event(true,&event));
    io.AddKeyEvent(ImGuiKey_Enter,true); frame(ui);
    char rgb[16]; CHECK(arcade_colors_get_text(nullptr,&color,rgb,sizeof rgb));
    CHECK(!std::strcmp(rgb,"#123ABC") && !recomp_runtime_ui_wants_text_input(ui));
    io.AddKeyEvent(ImGuiKey_Enter,false); frame(ui);
    state.shortcut[CYC_SC_MENU]=true;
    cyc_ui_text_filter_shortcuts(false,&wait_release,&state);
    CHECK(wait_release && !state.shortcut[CYC_SC_MENU]);
    cyc_ui_text_filter_shortcuts(false,&wait_release,&state); CHECK(!wait_release);
    state.shortcut[CYC_SC_MENU]=true;
    cyc_ui_text_filter_shortcuts(false,&wait_release,&state); CHECK(state.shortcut[CYC_SC_MENU]);

    /* Esc cancels only the RGB edit and leaves the pause menu open. */
    CHECK(recomp_runtime_ui_handle_input(ui,RECOMP_RUNTIME_UI_INPUT_ACCEPT,1,0));
    frame(ui); frame(ui);
    io.AddInputCharactersUTF8("#FFFFFF"); frame(ui);
    CHECK(!std::strcmp(ui->edit_buffer,"#FFFFFF"));
    event=key_event(SDLK_ESCAPE,SDL_SCANCODE_ESCAPE);
    CHECK(cyc_ui_text_owns_event(true,&event));
    io.AddKeyEvent(ImGuiKey_Escape,true); frame(ui);
    CHECK(recomp_runtime_ui_is_open(ui) && !recomp_runtime_ui_wants_text_input(ui));
    CHECK(arcade_colors_get_text(nullptr,&color,rgb,sizeof rgb) && !std::strcmp(rgb,"#123ABC"));
    io.AddKeyEvent(ImGuiKey_Escape,false); frame(ui);
    event.type=SDL_QUIT; CHECK(!cyc_ui_text_owns_event(true,&event));
    event.type=SDL_WINDOWEVENT; CHECK(!cyc_ui_text_owns_event(true,&event));
    event.type=SDL_CONTROLLERDEVICEREMOVED; CHECK(!cyc_ui_text_owns_event(true,&event));
    event.type=SDL_CONTROLLERBUTTONDOWN; CHECK(cyc_ui_text_owns_event(true,&event));
    event=key_event(SDLK_RETURN,SDL_SCANCODE_RETURN); CHECK(!cyc_ui_text_owns_event(false,&event));
    recomp_runtime_ui_destroy(ui);
    CHECK(wheel_editor()==0);
    ImGui::DestroyContext();
    std::puts("PASS RGB text compatibility and visual wheel: real mouse selection, live sprite preview, cancel/save and 640x480 layout.");
    return 0;
}
