#include "native_mode.h"
#undef NDEBUG
#include <assert.h>

enum { DEMO = 0x30, GAMEPLAY = 0x40, P1_OUT = 0x4a, P2_OUT = 0x4e };
static uint8_t ram[0x100];

static uint8_t peek(uint16_t address) { return ram[address]; }

static void game_over_return(int mode, bool second_player_active) {
    ArcadeModeLaunch launch;
    arcade_mode_begin(&launch, mode);
    ram[DEMO] = 1;
    ram[GAMEPLAY] = 11;
    ram[P1_OUT] = ram[P2_OUT] = 0xff;
    assert(!arcade_mode_game_over_complete(&launch, peek));

    launch.status = ARCADE_MODE_STARTED;
    assert(!arcade_mode_game_over_complete(&launch, peek));  /* No match was observed. */

    ram[DEMO] = 0;
    ram[GAMEPLAY] = 4;
    ram[P1_OUT] = 0;
    ram[P2_OUT] = second_player_active ? 0 : 0xff;
    assert(!arcade_mode_game_over_complete(&launch, peek));
    if (second_player_active) {
        ram[P1_OUT] = 0xff;
        assert(!arcade_mode_game_over_complete(&launch, peek));
        assert(!launch.game_over_seen);
    }

    ram[GAMEPLAY] = 11;
    ram[P1_OUT] = ram[P2_OUT] = 0xff;
    for (unsigned i = 0; i < 350; ++i)
        assert(!arcade_mode_game_over_complete(&launch, peek));  /* Wait for the ROM. */
    assert(launch.game_over_seen);

    ram[DEMO] = 1;
    assert(arcade_mode_game_over_complete(&launch, peek));
    assert(!arcade_mode_game_over_complete(&launch, peek));
}

int main(void) {
    game_over_return(ARCADE_MODE_1_PLAYER_A, false);
    game_over_return(ARCADE_MODE_1_PLAYER_B, false);
    game_over_return(ARCADE_MODE_2_PLAYERS_A, true);
    game_over_return(ARCADE_MODE_2_PLAYERS_B, true);
    return 0;
}
