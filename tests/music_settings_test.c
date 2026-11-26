#include "cyc_settings.h"
#include "cyc_arcade_filter.h"
#include <stdio.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
    CycSettings s, loaded;
    cyc_settings_default(&s);
    CHECK(s.music_enabled == 0 && s.music_volume == 35);
    CHECK(s.frame_mode == 0 && s.frame_rgb == 0xFFB02Bu);
    CHECK(s.visual_filter == 0 && s.grid_intensity == 35);
    CHECK(s.scanline_intensity == 35 && s.crt_intensity == 35);
    s.music_enabled = 1; s.music_volume = 64;
    s.show_fps = 1; s.fps_target = 120;
    s.frame_mode = 1; s.frame_rgb = 0x123ABCu;
    s.visual_filter = ARCADE_FILTER_CRT; s.grid_intensity = 76;
    s.scanline_intensity = 20; s.crt_intensity = 55;
    CHECK(cyc_settings_save(&s, "music-settings-test.ini", NULL));
    cyc_settings_default(&loaded);
    CHECK(cyc_settings_load(&loaded, "music-settings-test.ini", stderr, NULL));
    CHECK(loaded.music_enabled == 1 && loaded.music_volume == 64);
    CHECK(loaded.show_fps == 1 && loaded.fps_target == 120);
    CHECK(loaded.frame_mode == 1 && loaded.frame_rgb == 0x123ABCu);
    CHECK(loaded.visual_filter == ARCADE_FILTER_CRT && loaded.grid_intensity == 76);
    CHECK(loaded.scanline_intensity == 20 && loaded.crt_intensity == 55);
    FILE *f = fopen("music-settings-test.ini", "wb"); CHECK(f);
    fputs("[Audio]\nMusicEnabled = 0\nMusicVolume = 999\n", f); fclose(f);
    CHECK(cyc_settings_load(&loaded, "music-settings-test.ini", stderr, NULL));
    CHECK(loaded.music_enabled == 0 && loaded.music_volume == 100);
    CHECK(loaded.frame_mode == 1 && loaded.frame_rgb == 0x123ABCu);
    CHECK(loaded.visual_filter == ARCADE_FILTER_CRT && loaded.grid_intensity == 76);
    f = fopen("music-settings-test.ini", "wb"); CHECK(f);
    fputs("[Display]\nFrameMode = 2\nFrameColor = #12345Z\n"
          "VisualFilter = 4294967296\nGridIntensity = 101\n"
          "ScanlineIntensity = -1\nCrtIntensity = 101\n", f); fclose(f);
    CHECK(cyc_settings_load(&loaded, "music-settings-test.ini", NULL, NULL));
    CHECK(loaded.frame_mode == 1 && loaded.frame_rgb == 0x123ABCu);
    CHECK(loaded.visual_filter == ARCADE_FILTER_CRT && loaded.grid_intensity == 76);
    CHECK(loaded.scanline_intensity == 20 && loaded.crt_intensity == 55);
    f = fopen("music-settings-test.ini", "wb"); CHECK(f);
    fputs("[Display]\nFrameMode = 0\nFrameColor = #000000\n"
          "VisualFilter = 1\nGridIntensity = 0\n", f); fclose(f);
    cyc_settings_default(&loaded);
    CHECK(cyc_settings_load(&loaded, "music-settings-test.ini", stderr, NULL));
    CHECK(loaded.frame_mode == 0 && loaded.frame_rgb == 0);
    CHECK(loaded.visual_filter == ARCADE_FILTER_GRID && loaded.grid_intensity == 0);
    CHECK(loaded.scanline_intensity == 35 && loaded.crt_intensity == 35);
    CHECK(!arcade_filter_intensity_name(ARCADE_FILTER_NONE) && !arcade_filter_intensity_name(ARCADE_FILTER_SHARP));
    const uint32_t source[] = {1, 2, 3, 4};
    uint32_t expanded[20] = {0};
    const uint32_t expected[] = {1, 1, 2, 2, 0, 1, 1, 2, 2, 0, 3, 3, 4, 4, 0, 3, 3, 4, 4, 0};
    arcade_expand_2x(expanded, 5, source, 2, 2);
    CHECK(!memcmp(expanded, expected, sizeof(expanded)));
    CHECK(!arcade_sharp_fractional(1024, 960, 256, 240));
    CHECK(arcade_sharp_fractional(1000, 938, 256, 240));
    remove("music-settings-test.ini");
    puts("PASS audio, filter preferences, older Grid settings and sharp-scaling geometry.");
    return 0;
}
