#include "music_gate.h"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    ArcadeMusicGate g={0}; ArcadeMusicSnapshot s={0};
    s.demo=1; CHECK(!(arcade_music_gate_update(&g,&s,1)&ARCADE_MUSIC_PLAY));
    s.demo=0;s.phase=1;s.mode=8;s.jingle=2;s.mario_active=1;
    CHECK(arcade_music_gate_update(&g,&s,1)&ARCADE_MUSIC_RESET);
    s.mode=4;CHECK(!(arcade_music_gate_update(&g,&s,1)&ARCADE_MUSIC_PLAY));
    s.jingle=0;CHECK(!(arcade_music_gate_update(&g,&s,0)&ARCADE_MUSIC_PLAY));
    CHECK(arcade_music_gate_update(&g,&s,1)&ARCADE_MUSIC_PLAY);
    s.mode=5;CHECK(!(arcade_music_gate_update(&g,&s,0)&ARCADE_MUSIC_PLAY));
    s.mode=10;s.pending=8;CHECK(!(arcade_music_gate_update(&g,&s,1)&ARCADE_MUSIC_RESET));
    s.pending=0;s.mode=4;s.jingle=1;CHECK(!(arcade_music_gate_update(&g,&s,1)&ARCADE_MUSIC_PLAY));
    s.jingle=0;CHECK(arcade_music_gate_update(&g,&s,1)&ARCADE_MUSIC_PLAY);
    s.mario_state=0x10;CHECK(arcade_music_gate_update(&g,&s,0)&ARCADE_MUSIC_RESET);
    s.mario_state=4;CHECK(!(arcade_music_gate_update(&g,&s,1)&ARCADE_MUSIC_PLAY));
    s.mario_state=0;CHECK(arcade_music_gate_update(&g,&s,1)&ARCADE_MUSIC_PLAY);
    s.complete=1;CHECK(!(arcade_music_gate_update(&g,&s,0)&ARCADE_MUSIC_PLAY));
    s.phase=2;s.complete=0;s.mode=3;CHECK(arcade_music_gate_update(&g,&s,1)&ARCADE_MUSIC_RESET);
    s.mode=4;s.pending=2;CHECK(arcade_music_gate_update(&g,&s,1)&ARCADE_MUSIC_RESET);
    CHECK(!g.ready);
    s.pending=0;s.jingle=1;CHECK(!(arcade_music_gate_update(&g,&s,1)&ARCADE_MUSIC_PLAY));
    s.jingle=0;CHECK(arcade_music_gate_update(&g,&s,1)&ARCADE_MUSIC_PLAY);
    s.mario_state=0x10;s.luigi_active=1;s.luigi_state=0;
    CHECK(arcade_music_gate_update(&g,&s,0)&ARCADE_MUSIC_PLAY);
    s.mode=11;CHECK(!(arcade_music_gate_update(&g,&s,0)&ARCADE_MUSIC_PLAY));
    s.demo=1;CHECK(arcade_music_gate_update(&g,&s,1)&ARCADE_MUSIC_RESET);
    CHECK(g.phase==0&&!g.ready);
    puts("PASS title, intros, native pause, deaths, phase change, two players and game over.");
    return 0;
}
