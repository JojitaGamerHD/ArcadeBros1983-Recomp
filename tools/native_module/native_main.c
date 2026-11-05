#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ARCADE_NATIVE_MENU
#include <SDL.h>
#include <SDL_syswm.h>
#include "native_import.h"
#include "native_menu.h"
#include "native_mode.h"
#include "native_title_audio.h"

static bool integrated_menu;
static SDL_Window *game_window;
static bool rom_ready;
static int selected_mode = ARCADE_MODE_NONE;
static const char *selected_rom;

bool arcade_menu_active(void) { return integrated_menu; }
SDL_Window **arcade_menu_window(void) { return &game_window; }
bool arcade_menu_rom_ready(void) { return rom_ready; }
int arcade_menu_mode(void) { return selected_mode; }
const char *arcade_menu_rom(void) { return selected_rom; }

static HWND menu_owner(void)
{
    SDL_Window *window = SDL_GetKeyboardFocus();
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (window && SDL_GetWindowWMInfo(window, &info) && info.subsystem == SDL_SYSWM_WINDOWS)
        return info.info.win.window;
    return NULL;
}

int arcade_menu_action(int action)
{
    HWND owner = menu_owner();
    ArcadeImportPaths paths = {0};
    ArcadeImportStatus status = action < 0 ? arcade_import_select(owner, &paths)
                                           : arcade_import_restore(owner, &paths);
    if (status != ARCADE_IMPORT_READY) return status;
    rom_ready = true;
    arcade_title_audio_set_rom(paths.rom);
    if (action < 0) return ARCADE_IMPORT_READY;

    char error[512];
    if (!arcade_module_load(paths.module, error, sizeof(error))) {
        fprintf(stderr, "[arcade module] %s\n", error);
        if (owner) MessageBoxA(owner, error, "Arcade Bros 1983 Recomp", MB_OK | MB_ICONERROR);
        return ARCADE_IMPORT_ERROR;
    }
    selected_rom = paths.rom;
    selected_mode = action;
    fputs("[arcade module] ABI v1 accepted; using locally compiled game code.\n", stderr);
    return ARCADE_IMPORT_READY;
}

void arcade_menu_end_game(void)
{
    arcade_module_close();
    selected_rom = NULL;
    selected_mode = ARCADE_MODE_NONE;
}

static bool use_data_directory(const char *path)
{
    if (!path) return false;
    int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
    if (!size) return false;
    wchar_t *wide = malloc((size_t)size * sizeof(*wide));
    if (!wide) return false;
    bool ok = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, size) &&
              SetCurrentDirectoryW(wide);
    free(wide);
    return ok;
}
#endif

int arcade_host_main(int argc, char **argv);

int main(int argc, char **argv) {
#ifdef ARCADE_NATIVE_MENU
    const int prepare = arcade_import_prepare_only();
    if (prepare >= 0) return prepare;
    SDL_SetMainReady();
    if (argc == 1) {
        ArcadeImportPaths paths = {0};
        rom_ready = arcade_import_cached(&paths) == ARCADE_IMPORT_READY;
        if (!use_data_directory(paths.data) || !paths.config) {
            MessageBoxW(NULL, L"No se pudo abrir la carpeta local Datos.", L"Arcade Bros 1983 Recomp",
                        MB_OK | MB_ICONERROR);
            return 2;
        }
        char config_option[] = "--config";
        char *config = _strdup(paths.config);
        if (!config) return 2;
        char *args[] = {argv[0], config_option, config, NULL};
        arcade_title_audio_set_rom(paths.rom);
        integrated_menu = true;
        int result = arcade_host_main(3, args);
        arcade_menu_end_game();
        arcade_title_audio_set_rom(NULL);
        if (game_window) SDL_DestroyWindow(game_window);
        SDL_Quit();
        free(config);
        return result;
    }
#endif
    if (argc < 3) {
        fprintf(stderr, "usage: %s <local-game.dll> <rom.nes> [headless host options]\n", argv[0]);
        return 2;
    }
    char error[512];
    if (!arcade_module_load(argv[1], error, sizeof(error))) {
        fprintf(stderr, "[arcade module] %s\n", error);
        return 2;
    }
    fputs("[arcade module] ABI v1 accepted; using locally compiled game code.\n", stderr);
    for (int i = 1; i + 1 < argc; ++i) argv[i] = argv[i + 1];
    argv[argc - 1] = NULL;
    int result = arcade_host_main(argc - 1, argv);
    arcade_module_close();
    return result;
}
