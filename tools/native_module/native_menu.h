#ifndef ARCADE_NATIVE_MENU_H
#define ARCADE_NATIVE_MENU_H

#include <stdbool.h>

struct SDL_Window;

bool arcade_menu_active(void);
struct SDL_Window **arcade_menu_window(void);
bool arcade_menu_rom_ready(void);
int arcade_menu_action(int action);
int arcade_menu_mode(void);
const char *arcade_menu_rom(void);
void arcade_menu_end_game(void);

#endif
