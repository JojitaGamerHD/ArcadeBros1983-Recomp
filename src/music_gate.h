#pragma once
#include <stdint.h>
#include <string.h>

/* Host-only policy for the supported Mario Bros. (World) NES revision.
 * RAM roles cross-checked against the supplied ROM and RussianManSMWC's
 * Mario-Bros.-Dis/Defines/{RAM,Enums,Constants}.asm. No ROM writes.
 */
typedef struct {
    uint8_t demo, mode, phase, complete, jingle, pending;
    uint8_t current_jingle;
    uint8_t mario_active, mario_state, luigi_active, luigi_state;
} ArcadeMusicSnapshot;
typedef struct { uint8_t phase; int ready, started; } ArcadeMusicGate;
enum { ARCADE_MUSIC_PLAY = 1, ARCADE_MUSIC_RESET = 2 };

static inline int arcade_music_gate_update(ArcadeMusicGate *g,
                                           const ArcadeMusicSnapshot *s,
                                           int before_frame) {
    int action = 0;
    if (s->demo || !s->phase) {
        if (g->phase) action |= ARCADE_MUSIC_RESET;
        memset(g, 0, sizeof(*g));
        return action;
    }
    if (s->phase != g->phase) {
        g->phase = s->phase;
        g->ready = 0;
        g->started = 0;
        action |= ARCADE_MUSIC_RESET;
    }
    // A native game/phase intro also marks a new level entry, even if a
    // displayed phase counter eventually repeats or stops increasing.
    if (s->pending & 0x03) {
        action |= ARCADE_MUSIC_RESET;
        g->started = g->ready = 0;
    }
    // ARCADE_CONTINUOUS_LIFE_MUSIC_V090: Game Over cuts immediately, while
    // a lost life and the reappearance platform preserve the same cursor.
    if (s->mode == 11 || (s->pending & 0x40)) {
        g->started = g->ready = 0;
        return action;
    }
    if (s->complete || (s->mode != 4 && s->mode != 5 && s->mode != 10))
        g->started = 0;
    const int alive = (s->mario_active && s->mario_state == 0) ||
                      (s->luigi_active && s->luigi_state == 0);
    // Mode 4 is active play; modes 5/10 pause/unpause, 8 intro, 9 phase-end,
    // 6 bonus results and 11 game-over must not receive a music overlay.
    // Once a phase has started, only the reappearance jingle (index 4,
    // pending bit 0x10) may overlap the music. Keep other jingle policies.
    const int sound_ok = g->started
        ? (!s->jingle || s->current_jingle == 4) && !(s->pending & ~0x10u)
        : alive && !s->jingle && !s->pending;
    const int eligible = s->mode == 4 && !s->complete && sound_ok;
    if (!eligible) g->ready = 0;
    else if (before_frame) g->ready = g->started = 1;
    // Opening only before a frame avoids mixing music over the final intro
    // samples produced in the frame that clears the jingle flag.
    return action | (g->ready ? ARCADE_MUSIC_PLAY : 0);
}
