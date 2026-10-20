cmake_minimum_required(VERSION 3.20)

project(ArcadeBros1983Recomp LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

get_filename_component(NESRECOMP_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../nesrecomp"
    ABSOLUTE
)

set(NESRECOMP_RECOMP_UI OFF CACHE STRING "" FORCE)

include("${NESRECOMP_ROOT}/runner/cyc/project.cmake")

nesrecomp_add_cycle_game(ArcadeBros1983Recomp
    ROM "${CMAKE_CURRENT_SOURCE_DIR}/roms/Mario Bros..nes"
    GAME_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/game.toml"
    RECOMPILER "${NESRECOMP_ROOT}/build/recompiler/Release/NESRecomp.exe"
    PLAYERS 2
    REGION NTSC
)
