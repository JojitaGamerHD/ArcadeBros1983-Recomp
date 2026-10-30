#include "music.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(int argc, char **argv) {
    CHECK(argc == 2);
    CHECK(!arcade_music_load_utf8("missing-music-test-file.m4a"));
    std::vector<int16_t> unchanged(512, 1234);
    arcade_music_mix(unchanged.data(), unchanged.size(), 1, 35);
    CHECK(std::all_of(unchanged.begin(),unchanged.end(),[](int16_t x){return x==1234;}));
    FILE *track = std::fopen(argv[1], "rb");
    if (!track) {
        std::puts("SKIP: no private test track; music file is not distributed.");
        return 77;
    }
    std::fclose(track);
    CHECK(arcade_music_load_utf8(argv[1]));
    const size_t frames = arcade_music_frame_count();
    CHECK(frames > 0 && frames <= 32u*1024u*1024u);
    std::vector<int16_t> raw(frames, 0);
    arcade_music_mix(raw.data(), raw.size(), 1, 100);
    CHECK(arcade_music_cursor() == 0);
    CHECK(std::any_of(raw.begin(),raw.end(),[](int16_t x){return x>100;}));
    CHECK(std::any_of(raw.begin(),raw.end(),[](int16_t x){return x< -100;}));
    std::vector<int16_t> positive(frames,32767);
    arcade_music_mix(positive.data(),positive.size(),1,100);
    for(size_t i=0;i<frames;++i) CHECK(positive[i] == std::clamp(32767+int(raw[i]),-32768,32767));
    std::vector<int16_t> negative(frames,-32768);
    arcade_music_mix(negative.data(),negative.size(),1,100);
    for(size_t i=0;i<frames;++i) CHECK(negative[i] == std::clamp(-32768+int(raw[i]),-32768,32767));
    arcade_music_mix(unchanged.data(),unchanged.size(),0,100);
    CHECK(arcade_music_cursor()==0);
    CHECK(std::all_of(unchanged.begin(),unchanged.end(),[](int16_t x){return x==1234;}));
    arcade_music_mix(unchanged.data(),unchanged.size(),1,0);
    CHECK(arcade_music_cursor()==unchanged.size());
    CHECK(std::all_of(unchanged.begin(),unchanged.end(),[](int16_t x){return x==1234;}));
    arcade_music_reset();
    std::vector<int16_t> wrapped(frames+37,0);
    arcade_music_mix(wrapped.data(),wrapped.size(),1,100);
    CHECK(arcade_music_cursor()==37);
    for(size_t i=0;i<wrapped.size();++i) CHECK(wrapped[i]==raw[i%frames]);
    CHECK(arcade_music_load_utf8(argv[1]));
    CHECK(arcade_music_cursor()==0);
    CHECK(!arcade_music_load_utf8(nullptr));
    CHECK(arcade_music_frame_count()==0);
    std::printf("PASS real track decoded: %.3f seconds; mix, saturation, mute, reload and wrap checked.\n", double(frames)/48000);
    return 0;
}
