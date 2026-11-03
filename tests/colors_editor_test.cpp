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
    ImGui::DestroyContext(); recomp_runtime_ui_destroy(ui);
    std::puts("PASS RGB widget: Backspace, arrows, Enter, Esc, shortcut release and host lifecycle events.");
    return 0;
}
