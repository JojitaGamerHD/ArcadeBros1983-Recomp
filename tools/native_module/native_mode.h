#ifndef ARCADE_NATIVE_MODE_H
#define ARCADE_NATIVE_MODE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ARCADE_MODE_NONE = -1,
    ARCADE_MODE_1_PLAYER_A = 0,
    ARCADE_MODE_1_PLAYER_B,
    ARCADE_MODE_2_PLAYERS_A,
    ARCADE_MODE_2_PLAYERS_B
} ArcadeGameMode;

typedef enum {
    ARCADE_MODE_IDLE,
    ARCADE_MODE_PENDING,
    ARCADE_MODE_STARTED,
    ARCADE_MODE_TIMED_OUT
} ArcadeModeStatus;

typedef struct {
    ArcadeModeStatus status;
    unsigned frames;
    int mode;
    bool start_sent;
    bool game_over_seen;
} ArcadeModeLaunch;

/* Initialize once per new emulated session. NONE preserves direct input. */
void arcade_mode_begin(ArcadeModeLaunch *launch, int mode);

/* Call once per simulated frame before latching the two controller bytes.
 * The terminal status persists until the next call to arcade_mode_begin. */
ArcadeModeStatus arcade_mode_input(ArcadeModeLaunch *launch,
    uint8_t (*peek)(uint16_t address), uint8_t buttons[2]);

/* Call after each simulated frame. The original match has finished once
 * its Game Over sequence hands control back to the title screen. */
bool arcade_mode_game_over_complete(ArcadeModeLaunch *launch,
    uint8_t (*peek)(uint16_t address));

#ifdef __cplusplus
}
#endif

#endif
