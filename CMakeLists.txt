cmake_minimum_required(VERSION 3.20)

project(ArcadeBros1983Recomp LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Motor NESRecomp independiente con parche Arcade v0.6.0
get_filename_component(NESRECOMP_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/nesrecomp-arcade"
    ABSOLUTE
)
# Idioma del launcher de Arcade Bros 1983 Recomp
set(RECOMP_UI_LANGUAGE "es" CACHE STRING "Idioma de recomp-ui" FORCE)
set(NESRECOMP_RECOMP_UI "" CACHE STRING "" FORCE)

include("${NESRECOMP_ROOT}/runner/cyc/project.cmake")

nesrecomp_add_cycle_game(ArcadeBros1983Recomp
    ROM "${CMAKE_CURRENT_SOURCE_DIR}/roms/Mario Bros..nes"
    GAME_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/game.toml"
    RECOMPILER "${NESRECOMP_ROOT}/build/recompiler/Release/NESRecomp.exe"
    PLAYERS 2
    REGION NTSC
    HOST_EXTRAS "${CMAKE_CURRENT_SOURCE_DIR}/src/records.c"
)

# Lanzador portable de Arcade Bros 1983 Recomp
if(WIN32)
    add_executable(ArcadeBrosLauncher WIN32 src/launcher.cpp)
    target_compile_features(ArcadeBrosLauncher PRIVATE cxx_std_17)
    target_link_libraries(ArcadeBrosLauncher PRIVATE comdlg32 bcrypt)
endif()
# ARCADE_PIXEL_FONT_V050
# Fuente pixelada exclusiva del menu de pausa.
add_custom_command(TARGET ArcadeBros1983Recomp POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "$<TARGET_FILE_DIR:ArcadeBros1983Recomp>/assets/fonts"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/PixelifySans.ttf"
        "$<TARGET_FILE_DIR:ArcadeBros1983Recomp>/assets/fonts/PixelifySans.ttf"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/OFL-PixelifySans.txt"
        "$<TARGET_FILE_DIR:ArcadeBros1983Recomp>/assets/fonts/OFL-PixelifySans.txt"
    COMMENT "Copiando tipografia pixelada de Arcade Bros"
    VERBATIM
)

# ARCADE_BOXART_V060
# Portada del launcher Arcade Bros 1983 Recomp.
add_custom_command(TARGET ArcadeBros1983Recomp POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "$<TARGET_FILE_DIR:ArcadeBros1983Recomp>/assets/img"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/img/arcade_bros_1983.png"
        "$<TARGET_FILE_DIR:ArcadeBros1983Recomp>/assets/img/arcade_bros_1983.png"
    COMMENT "Copiando portada Arcade Bros 1983"
    VERBATIM
)
