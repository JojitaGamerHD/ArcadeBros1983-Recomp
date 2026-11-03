#include "recomp_runtime_ui.h"
const RecompRuntimeUiItem *test_color_menu(size_t *count) {
    static const RecompRuntimeUiItem items[]={
#include "../src/player_colors_menu.inc"
    };
    *count=sizeof items/sizeof items[0];
    return items;
}
