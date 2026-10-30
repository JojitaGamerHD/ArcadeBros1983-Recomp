cmake_minimum_required(VERSION 3.20)
project(ArcadeMusicTests LANGUAGES C CXX)

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Estas pruebas requieren Windows x64, igual que el juego.")
endif()

get_filename_component(ARCADE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(CYC_ROOT "${ARCADE_ROOT}/deps/nesrecomp-arcade/runner/cyc")
set(SDL_ROOT "${ARCADE_ROOT}/deps/nesrecomp-arcade/runner/external/SDL2")
if(NOT EXISTS "${CYC_ROOT}/cyc_settings.c" OR NOT EXISTS "${SDL_ROOT}/lib/x64/SDL2.lib")
    message(FATAL_ERROR "Prepara primero las dependencias con tools/preparar_dependencias.ps1.")
endif()

enable_testing()

add_executable(smooth_video_test smooth_video_test.cpp "${ARCADE_ROOT}/src/smooth_video.cpp")
target_compile_features(smooth_video_test PRIVATE cxx_std_17)
target_include_directories(smooth_video_test PRIVATE "${ARCADE_ROOT}/src")
if(MSVC)
    target_compile_options(smooth_video_test PRIVATE /utf-8 /W4)
endif()
add_test(NAME smooth_video_geometry COMMAND smooth_video_test)

add_executable(music_mix_test music_mix_test.cpp "${ARCADE_ROOT}/src/music.cpp")
target_compile_features(music_mix_test PRIVATE cxx_std_17)
target_include_directories(music_mix_test PRIVATE "${ARCADE_ROOT}/src")
target_link_libraries(music_mix_test PRIVATE mfplat mfreadwrite mfuuid ole32)
set(ARCADE_TEST_TRACK "${ARCADE_ROOT}/assets/music/stage1.m4a" CACHE FILEPATH
    "Pista privada utilizada para comprobar decodificacion y mezcla; nunca se descarga.")
add_test(NAME music_decode_and_mix COMMAND music_mix_test "${ARCADE_TEST_TRACK}")
set_tests_properties(music_decode_and_mix PROPERTIES SKIP_RETURN_CODE 77)

add_executable(music_gate_test music_gate_test.c)
target_compile_features(music_gate_test PRIVATE c_std_11)
target_include_directories(music_gate_test PRIVATE "${ARCADE_ROOT}/src")
add_test(NAME music_phase_gate COMMAND music_gate_test)

add_executable(music_settings_test music_settings_test.c
    "${CYC_ROOT}/cyc_settings.c" "${CYC_ROOT}/cyc_input.c")
target_compile_features(music_settings_test PRIVATE c_std_11)
target_compile_definitions(music_settings_test PRIVATE SDL_MAIN_HANDLED _CRT_SECURE_NO_WARNINGS)
target_include_directories(music_settings_test PRIVATE "${CYC_ROOT}" "${SDL_ROOT}/include")
target_link_libraries(music_settings_test PRIVATE "${SDL_ROOT}/lib/x64/SDL2.lib")
add_custom_command(TARGET music_settings_test POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${SDL_ROOT}/lib/x64/SDL2.dll" "$<TARGET_FILE_DIR:music_settings_test>"
    VERBATIM)
add_test(NAME music_settings_persistence COMMAND music_settings_test)

if(MSVC)
    foreach(target music_mix_test music_gate_test music_settings_test)
        target_compile_options(${target} PRIVATE /utf-8)
    endforeach()
endif()
