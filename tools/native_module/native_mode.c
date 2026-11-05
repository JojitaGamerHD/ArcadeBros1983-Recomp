#include "native_mode.h"

enum {
    TITLE_SELECT_HELD = 0x28,
    TITLE_CURSOR = 0x29,
    DEMO_FLAG = 0x30,
    GAMEPLAY_MODE = 0x40,
    PLAYER1_GAME_OVER = 0x4a,
    PLAYER2_GAME_OVER = 0x4e,
    NON_GAMEPLAY_MODE = 0x50,
    TITLE_SCREEN = 1,
    GAME_OVER = 11,
    NES_START = 0x10,
    NES_SELECT = 0x20,
    MAX_START_FRAMES = 300
};

void arcade_mode_begin(ArcadeModeLaunch *launch, int mode) {
    *launch = (ArcadeModeLaunch){
        .status = mode >= ARCADE_MODE_1_PLAYER_A && mode <= ARCADE_MODE_2_PLAYERS_B
            ? ARCADE_MODE_PENDING : ARCADE_MODE_IDLE,
        .mode = mode
    };
}

ArcadeModeStatus arcade_mode_input(ArcadeModeLaunch *launch,
    uint8_t (*peek)(uint16_t address), uint8_t buttons[2]) {
    if (launch->status != ARCADE_MODE_PENDING) return launch->status;

    buttons[0] = buttons[1] = 0;
    const bool demo = peek(DEMO_FLAG) != 0;
    if (launch->start_sent && !demo) {
        launch->status = ARCADE_MODE_STARTED;
    } else if (launch->frames == MAX_START_FRAMES) {
        launch->status = ARCADE_MODE_TIMED_OUT;
    } else {
        ++launch->frames;
        /* The supported ROM initializes the cursor during boot, then reads
         * Select/Start on the title screen. Wait for its own input handler. */
        if (demo && peek(NON_GAMEPLAY_MODE) == TITLE_SCREEN) {
            if (peek(TITLE_CURSOR) == launch->mode) {
                buttons[0] = NES_START;
                launch->start_sent = true;
            } else if (!peek(TITLE_SELECT_HELD)) {
                buttons[0] = NES_SELECT;
            }
        }
    }
    return launch->status;
}

bool arcade_mode_game_over_complete(ArcadeModeLaunch *launch,
    uint8_t (*peek)(uint16_t address)) {
    if (launch->status != ARCADE_MODE_STARTED &&
        launch->status != ARCADE_MODE_TIMED_OUT) return false;

    /* CheckForGameOver_E26D requires both flags. In a one-player game the
     * original initialization already sets the absent player's flag. */
    const bool in_game_over = peek(GAMEPLAY_MODE) == GAME_OVER &&
        peek(PLAYER1_GAME_OVER) && peek(PLAYER2_GAME_OVER);
    if (!peek(DEMO_FLAG)) {
        launch->game_over_seen = in_game_over;
        return false;
    }

    /* GameOver_E28B sets the demo flag only after the original timer
     * expires. Stop there, before the guest draws its title screen. */
    const bool completed = launch->game_over_seen && in_game_over;
    launch->game_over_seen = false;
    return completed;
}
