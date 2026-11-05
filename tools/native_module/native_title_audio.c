#include "native_title_audio.h"
#include "protocol.h"
#include "cyc_core.h"
#include "cyc_mod.h"
#include "cyc_ring.h"
#include "cyc_run.h"
#include <SDL.h>
#include <stdlib.h>
#include <string.h>

enum {
    TITLE_AUDIO_RATE = 48000,
    TITLE_AUDIO_QUEUE_BYTES = TITLE_AUDIO_RATE * sizeof(int16_t) / 12,
    ORIGINAL_TITLE_MODE = 1,
    ORIGINAL_DEMO_MODE = 6,
    ORIGINAL_DEMO_END = 7,
    ROM_DEMO_FLAG = 0x30,
    ROM_NON_GAMEPLAY_MODE = 0x50,
    ROM_DEMO_INPUT = 0x55,
    MAX_CATCHUP_FRAMES = 4
};

static char *title_rom;
static SDL_AudioDeviceID device;
static uint64_t next_frame, frame_ticks;
static bool attempted, running, audio_failed, demo_visible, demo_admitted;
static bool previous_native;
static uint32_t previous_ring_frame;
static uint64_t previous_interp_cycles[4];

static void finish_emulation(void)
{
    if (!running) return;
    cyc_audio_enable(0);
    cyc_run_native = previous_native;
    cyc_ring_frame = previous_ring_frame;
    cyc_run_interp_rom_cycles = previous_interp_cycles[0];
    cyc_run_interp_ram_cycles = previous_interp_cycles[1];
    cyc_run_interp_prg_ram_cycles = previous_interp_cycles[2];
    cyc_run_interp_other_cycles = previous_interp_cycles[3];
    running = false;
}

static void close_device(void)
{
    if (device) SDL_CloseAudioDevice(device);
    device = 0;
}

void arcade_title_audio_stop(void)
{
    close_device();
    finish_emulation();
}

void arcade_title_audio_restart(void)
{
    arcade_title_audio_stop();
    attempted = audio_failed = demo_visible = demo_admitted = false;
}

void arcade_title_audio_set_rom(const char *path)
{
    arcade_title_audio_restart();
    free(title_rom);
    title_rom = path ? _strdup(path) : NULL;
}

static bool start_title(void)
{
    size_t size = 0;
    void *image = SDL_LoadFile(title_rom, &size);
    if (!image) return false;
    const bool loaded = cyc_load_ines(image, size);
    SDL_free(image);
    if (!loaded || cyc_prg_hash() != ARCADE_NATIVE_PRG_HASH) return false;

    previous_native = cyc_run_native;
    previous_ring_frame = cyc_ring_frame;
    previous_interp_cycles[0] = cyc_run_interp_rom_cycles;
    previous_interp_cycles[1] = cyc_run_interp_ram_cycles;
    previous_interp_cycles[2] = cyc_run_interp_prg_ram_cycles;
    previous_interp_cycles[3] = cyc_run_interp_other_cycles;
    cyc_run_native = false;
    cyc_set_console(CYC_CONSOLE_NES);
    cyc_set_region(CYC_REGION_NTSC);
    cyc_power_on(0);
    cyc_run_power_on();
    cyc_audio_enable(TITLE_AUDIO_RATE);
    frame_ticks = (uint64_t)(SDL_GetPerformanceFrequency() * cyc_frame_seconds());
    if (!frame_ticks) frame_ticks = 1;
    next_frame = SDL_GetPerformanceCounter();
    running = true;
    return true;
}

static void update_device(bool audio)
{
    if (!audio) {
        close_device();
        audio_failed = false;
        return;
    }
    if (device || audio_failed) return;
    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        audio_failed = true;
        return;
    }
    const SDL_AudioSpec want = {
        .freq = TITLE_AUDIO_RATE, .format = AUDIO_S16SYS, .channels = 1, .samples = 1024
    };
    device = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
    audio_failed = !device;
}

const uint32_t *arcade_title_audio_update(int enabled, int volume, int show_demo)
{
    if (!title_rom) return NULL;
    if (!attempted) {
        attempted = true;
        if (!start_title()) return NULL;
    }
    if (!running) return NULL;
    update_device(enabled && volume > 0);
    if (!show_demo) demo_admitted = false;
    const int gain = volume > 100 ? 100 : volume;
    const uint64_t now = SDL_GetPerformanceCounter();
    if (now > next_frame && now - next_frame > frame_ticks * MAX_CATCHUP_FRAMES)
        next_frame = now;

    /* Leave title timers, music and demo controller scripts to the ROM.
     * The same scheduler keeps attract mode moving even with audio muted. */
    for (unsigned step = 0; step < MAX_CATCHUP_FRAMES && now >= next_frame; ++step) {
        next_frame += frame_ticks;
        cyc_set_controller(0, 0);
        cyc_set_controller(1, 0);
        cyc_run_frame();
        const bool attract = cyc_mod_peek(ROM_DEMO_FLAG) != 0;
        const uint8_t mode = cyc_mod_peek(ROM_NON_GAMEPLAY_MODE);
        const bool was_visible = demo_visible;
        demo_visible = attract && ((mode == ORIGINAL_DEMO_MODE && cyc_mod_peek(ROM_DEMO_INPUT)) ||
                                  (was_visible && mode == ORIGINAL_DEMO_END));
        if (demo_visible && !was_visible) demo_admitted = show_demo != 0;
        int16_t pcm[2048];
        const size_t count = cyc_audio_read(pcm, sizeof(pcm) / sizeof(pcm[0]));
        if (device && attract && mode == ORIGINAL_TITLE_MODE && count &&
            SDL_GetQueuedAudioSize(device) < TITLE_AUDIO_QUEUE_BYTES) {
            for (size_t i = 0; i < count; ++i)
                pcm[i] = (int16_t)(pcm[i] * gain / 100);
            if (SDL_QueueAudio(device, pcm, (Uint32)(count * sizeof(pcm[0])))) {
                close_device();
                audio_failed = true;
            } else SDL_PauseAudioDevice(device, 0);
        }
    }
    return demo_visible && demo_admitted ? cyc_frame_argb() : NULL;
}