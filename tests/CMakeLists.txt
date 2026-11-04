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
add_test(NAME dependencies_patch_upgrade COMMAND powershell -NoProfile -ExecutionPolicy Bypass
    -File "${CMAKE_CURRENT_SOURCE_DIR}/patch_upgrade_test.ps1"
    -Root "${ARCADE_ROOT}" -Scratch "${CMAKE_CURRENT_BINARY_DIR}")

set(UI_ROOT "${ARCADE_ROOT}/recomp-ui/src")
include("${CYC_ROOT}/cyc.cmake")
add_executable(native_colors_test native_colors_test.cpp
    "${CYC_ROOT}/cpu6502.c" "${CYC_ROOT}/cpu6502_interp.c" "${CYC_ROOT}/hw_machine.c"
    "${CYC_ROOT}/hw_mapper.c" "${CYC_ROOT}/hw_fds.c" "${CYC_ROOT}/hw_fds_audio.c"
    "${CYC_ROOT}/hw_fds_hle.c" "${CYC_ROOT}/vendor/emu2413/emu2413.c"
    "${CYC_ROOT}/hw_ppu.c" "${CYC_ROOT}/hw_apu.c" "${CYC_ROOT}/hw_palette.c"
    "${CYC_ROOT}/cyc_trace.c" "${CYC_ROOT}/cyc_ring.c"
    ${NESRECOMP_CYC_SCHEDULER_SOURCES} "${CYC_ROOT}/cyc_native_none.c"
    "${CYC_ROOT}/cyc_video.c" "${CYC_ROOT}/cyc_render.c")
target_include_directories(native_colors_test PRIVATE "${ARCADE_ROOT}/src" ${NESRECOMP_CYC_INCLUDE_DIRS})
target_link_libraries(native_colors_test PRIVATE ${NESRECOMP_CYC_LIBRARIES})
target_compile_features(native_colors_test PRIVATE c_std_11 cxx_std_17)
target_compile_definitions(native_colors_test PRIVATE _CRT_SECURE_NO_WARNINGS)
if(MSVC)
    target_compile_options(native_colors_test PRIVATE /utf-8)
endif()
add_test(NAME native_colors_provenance COMMAND native_colors_test)
add_executable(colors_editor_test colors_editor_test.cpp color_menu_fixture.c "${ARCADE_ROOT}/src/player_colors.c"
    "${UI_ROOT}/common/recomp_runtime_ui.c" "${UI_ROOT}/common/recomp_runtime_settings.c"
    "${UI_ROOT}/third_party/imgui/imgui.cpp" "${UI_ROOT}/third_party/imgui/imgui_draw.cpp"
    "${UI_ROOT}/third_party/imgui/imgui_tables.cpp" "${UI_ROOT}/third_party/imgui/imgui_widgets.cpp"
    "${UI_ROOT}/third_party/imgui/backends/imgui_impl_sdl2.cpp"
    "${UI_ROOT}/third_party/imgui/backends/imgui_impl_sdlrenderer2.cpp")
target_include_directories(colors_editor_test PRIVATE "${ARCADE_ROOT}/src" "${UI_ROOT}"
    "${UI_ROOT}/common" "${UI_ROOT}/third_party/imgui" "${UI_ROOT}/third_party/imgui/backends"
    "${CYC_ROOT}" "${SDL_ROOT}/include")
target_compile_features(colors_editor_test PRIVATE c_std_11 cxx_std_17)
target_compile_definitions(colors_editor_test PRIVATE SDL_MAIN_HANDLED _CRT_SECURE_NO_WARNINGS)
target_link_libraries(colors_editor_test PRIVATE "${SDL_ROOT}/lib/x64/SDL2.lib")
add_custom_command(TARGET colors_editor_test POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${SDL_ROOT}/lib/x64/SDL2.dll" "$<TARGET_FILE_DIR:colors_editor_test>"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:colors_editor_test>/assets/fonts"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${ARCADE_ROOT}/assets/fonts/PixelifySans.ttf"
    "$<TARGET_FILE_DIR:colors_editor_test>/assets/fonts/PixelifySans.ttf")
if(MSVC)
    target_compile_options(colors_editor_test PRIVATE /utf-8)
endif()
add_test(NAME colors_editor_input COMMAND colors_editor_test)

set(CAPTURE_SOURCES common/launcher_model.c common/launcher_binds.c common/keybinds.c
    common/pad_binds.c consoles/nes/nes_binds.c consoles/psx/psx_binds.c
    consoles/psx/psx_pad_binds.c consoles/snes/snes_pad_binds.c consoles/n64/n64_binds.c
    consoles/n64/n64_pad_binds.c consoles/genesis/genesis_binds.c consoles/gb/gb_binds.c
    consoles/psx/memcard_format.c common/crc32.c common/sha1.c common/sha256.c common/ips_patch.c)
list(TRANSFORM CAPTURE_SOURCES PREPEND "${UI_ROOT}/")
add_executable(controls_capture_test "${ARCADE_ROOT}/recomp-ui/tests/nes_host_bindings_test.c" ${CAPTURE_SOURCES})
target_include_directories(controls_capture_test PRIVATE "${UI_ROOT}" "${UI_ROOT}/common" "${UI_ROOT}/consoles/psx" "${SDL_ROOT}/include")
target_compile_definitions(controls_capture_test PRIVATE SDL_MAIN_HANDLED _CRT_SECURE_NO_WARNINGS)
target_link_libraries(controls_capture_test PRIVATE "${SDL_ROOT}/lib/x64/SDL2.lib")
if(MSVC)
    target_compile_options(controls_capture_test PRIVATE /utf-8)
endif()
add_custom_command(TARGET controls_capture_test POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${SDL_ROOT}/lib/x64/SDL2.dll" "$<TARGET_FILE_DIR:controls_capture_test>")
add_test(NAME controls_capture COMMAND controls_capture_test "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(controls_colors_test controls_colors_test.c
    "${ARCADE_ROOT}/src/player_colors.c" "${CYC_ROOT}/cyc_settings.c" "${CYC_ROOT}/cyc_input.c")
target_include_directories(controls_colors_test PRIVATE "${ARCADE_ROOT}/src" "${ARCADE_ROOT}/recomp-ui/src" "${CYC_ROOT}" "${SDL_ROOT}/include")
target_compile_definitions(controls_colors_test PRIVATE SDL_MAIN_HANDLED _CRT_SECURE_NO_WARNINGS)
if(MSVC)
    target_compile_options(controls_colors_test PRIVATE /utf-8)
endif()
target_link_libraries(controls_colors_test PRIVATE "${SDL_ROOT}/lib/x64/SDL2.lib")
add_custom_command(TARGET controls_colors_test POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${SDL_ROOT}/lib/x64/SDL2.dll" "$<TARGET_FILE_DIR:controls_colors_test>")
add_test(NAME controls_colors_persistence COMMAND controls_colors_test)
set_tests_properties(controls_colors_persistence PROPERTIES FIXTURES_SETUP controls_colors_file)
add_test(NAME controls_colors_restart COMMAND controls_colors_test reload)
set_tests_properties(controls_colors_restart PROPERTIES FIXTURES_REQUIRED controls_colors_file)

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
