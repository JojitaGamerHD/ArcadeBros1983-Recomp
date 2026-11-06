#include "cyc_settings.h"
#include <stdio.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
    CycSettings s, loaded;
    cyc_settings_default(&s);
    CHECK(s.music_enabled == 0 && s.music_volume == 35);
    CHECK(s.frame_mode == 0 && s.frame_rgb == 0xFFB02Bu);
    s.music_enabled = 1; s.music_volume = 64;
    s.show_fps = 1; s.fps_target = 120;
    s.frame_mode = 1; s.frame_rgb = 0x123ABCu;
    CHECK(cyc_settings_save(&s, "music-settings-test.ini", NULL));
    cyc_settings_default(&loaded);
    CHECK(cyc_settings_load(&loaded, "music-settings-test.ini", stderr, NULL));
    CHECK(loaded.music_enabled == 1 && loaded.music_volume == 64);
    CHECK(loaded.show_fps == 1 && loaded.fps_target == 120);
    CHECK(loaded.frame_mode == 1 && loaded.frame_rgb == 0x123ABCu);
    FILE *f = fopen("music-settings-test.ini", "wb"); CHECK(f);
    fputs("[Audio]\nMusicEnabled = 0\nMusicVolume = 999\n", f); fclose(f);
    CHECK(cyc_settings_load(&loaded, "music-settings-test.ini", stderr, NULL));
    CHECK(loaded.music_enabled == 0 && loaded.music_volume == 100);
    CHECK(loaded.frame_mode == 1 && loaded.frame_rgb == 0x123ABCu);
    f = fopen("music-settings-test.ini", "wb"); CHECK(f);
    fputs("[Display]\nFrameMode = 2\nFrameColor = #12345Z\n", f); fclose(f);
    CHECK(cyc_settings_load(&loaded, "music-settings-test.ini", NULL, NULL));
    CHECK(loaded.frame_mode == 1 && loaded.frame_rgb == 0x123ABCu);
    f = fopen("music-settings-test.ini", "wb"); CHECK(f);
    fputs("[Display]\nFrameMode = 0\nFrameColor = #000000\n", f); fclose(f);
    CHECK(cyc_settings_load(&loaded, "music-settings-test.ini", stderr, NULL));
    CHECK(loaded.frame_mode == 0 && loaded.frame_rgb == 0);
    remove("music-settings-test.ini");
    puts("PASS audio and display preferences, bezel validation and volume clamp.");
    return 0;
}
