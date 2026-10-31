cmake_minimum_required(VERSION 3.21)
project(ArcadeNativeModuleProbe LANGUAGES C CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This development probe requires MSVC and Windows x64.")
endif()
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

get_filename_component(DEFAULT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(ARCADE_ROOT "${DEFAULT_ROOT}" CACHE PATH "Arcade Bros development checkout")
set(ENGINE "${ARCADE_ROOT}/deps/nesrecomp-arcade")
set(CYC "${ENGINE}/runner/cyc")
if(NOT EXISTS "${CYC}/cyc.cmake")
    message(FATAL_ERROR "Prepare the project dependencies first.")
endif()
option(ARCADE_PREPARE_MODULE "Generate a private module from the local ROM" ON)
option(ARCADE_WINDOWED_HOST "Build the generic SDL host and portable launcher" OFF)
set(ARCADE_RELEASE_VERSION "1.0.0" CACHE STRING "Arcade Bros release version")
if(ARCADE_WINDOWED_HOST AND ARCADE_PREPARE_MODULE)
    message(FATAL_ERROR "ARCADE_WINDOWED_HOST requires ARCADE_PREPARE_MODULE=OFF; the public host must not generate ROM code.")
endif()

include("${CYC}/cyc.cmake")
set_source_files_properties("${CYC}/cyc_host.c" PROPERTIES
    COMPILE_DEFINITIONS "main=arcade_host_main")
if(ARCADE_WINDOWED_HOST)
    # Compile the machine on this target so CYC_WITH_SDL reaches cyc_host.c.
    # The headless object library below deliberately has no SDL definitions.
    add_executable(ArcadeNativeHost ${NESRECOMP_CYC_SOURCES}
        native_bridge.c native_main.c runtime_exports.def
        "${ARCADE_ROOT}/src/records.c"
        "${ARCADE_ROOT}/src/music.cpp"
        "${ARCADE_ROOT}/src/smooth_host.c"
        "${ARCADE_ROOT}/src/smooth_video.cpp")
else()
    add_library(ArcadeMachine OBJECT ${NESRECOMP_CYC_SOURCES})
    target_compile_features(ArcadeMachine PRIVATE c_std_11)
    target_compile_definitions(ArcadeMachine PRIVATE _CRT_SECURE_NO_WARNINGS CYC_GAME_PLAYERS=2)
    target_compile_options(ArcadeMachine PRIVATE /utf-8)
    target_include_directories(ArcadeMachine PRIVATE ${NESRECOMP_CYC_INCLUDE_DIRS})

    # This target never links CYC_PROJECT_SOURCES or reads the ROM at configure time.
    add_executable(ArcadeNativeHost $<TARGET_OBJECTS:ArcadeMachine> native_bridge.c native_main.c runtime_exports.def)
endif()
target_compile_features(ArcadeNativeHost PRIVATE c_std_11)
target_compile_definitions(ArcadeNativeHost PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(ArcadeNativeHost PRIVATE /utf-8 /W4)
target_include_directories(ArcadeNativeHost PRIVATE ${NESRECOMP_CYC_INCLUDE_DIRS})
target_link_libraries(ArcadeNativeHost PRIVATE ${NESRECOMP_CYC_LIBRARIES})
set_target_properties(ArcadeNativeHost PROPERTIES ENABLE_EXPORTS TRUE)
target_sources(ArcadeNativeHost PRIVATE utf8.manifest)

if(ARCADE_WINDOWED_HOST)
    set(RECOMP_UI_ROOT "${ARCADE_ROOT}/recomp-ui" CACHE PATH "Arcade Bros recomp-ui checkout" FORCE)
    set(RECOMP_UI_LANGUAGE "es" CACHE STRING "Default launcher UI language code" FORCE)
    set(RECOMP_UI_SDL3 OFF CACHE BOOL "Use SDL2 for the generic runtime" FORCE)
    include("${RECOMP_UI_ROOT}/recomp_ui.cmake")
    target_compile_features(ArcadeNativeHost PRIVATE cxx_std_17)
    target_compile_definitions(ArcadeNativeHost PRIVATE
        CYC_GAME_PLAYERS=2 NESRECOMP_GAME_PLAYERS=2 CYC_GAME_REGION=0
        "CYC_BUILD_VERSION=\"${ARCADE_RELEASE_VERSION}\"")
    set_property(TARGET ArcadeNativeHost PROPERTY NESRECOMP_CYCLE_BACKEND TRUE)
    set_property(TARGET ArcadeNativeHost PROPERTY NESRECOMP_CYC_HOST_EXTRAS TRUE)
    nesrecomp_cyc_enable_sdl(ArcadeNativeHost)
    get_target_property(ARCADE_HAS_SDL ArcadeNativeHost NESRECOMP_CYC_SDL)
    if(NOT ARCADE_HAS_SDL)
        message(FATAL_ERROR "ARCADE_WINDOWED_HOST requires the prepared SDL2 dependency.")
    endif()
    nesrecomp_cyc_enable_recomp_ui(ArcadeNativeHost
        ROM_SHA256 "0cc334007d3eae698cdcd034d12ec9bab2b5266e85bc703cf24ccb4e2d63b654")
    target_link_libraries(ArcadeNativeHost PRIVATE mfplat mfreadwrite mfuuid ole32)
    add_custom_command(TARGET ArcadeNativeHost POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:ArcadeNativeHost>/assets/fonts"
            "$<TARGET_FILE_DIR:ArcadeNativeHost>/assets/img"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${ARCADE_ROOT}/assets/fonts/PixelifySans.ttf"
            "$<TARGET_FILE_DIR:ArcadeNativeHost>/assets/fonts/PixelifySans.ttf"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${ARCADE_ROOT}/assets/fonts/OFL-PixelifySans.txt"
            "$<TARGET_FILE_DIR:ArcadeNativeHost>/assets/fonts/OFL-PixelifySans.txt"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${ARCADE_ROOT}/assets/img/arcade_bros_1983.png"
            "$<TARGET_FILE_DIR:ArcadeNativeHost>/assets/img/arcade_bros_1983.png"
        VERBATIM)
    # User music remains private; no local OST is copied into the public host.
    add_executable(ArcadeBrosLauncher WIN32 portable_launcher.cpp utf8.manifest)
    target_compile_features(ArcadeBrosLauncher PRIVATE cxx_std_17)
    target_compile_options(ArcadeBrosLauncher PRIVATE /utf-8 /W4)
    target_compile_definitions(ArcadeBrosLauncher PRIVATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(ArcadeBrosLauncher PRIVATE comdlg32 bcrypt comctl32 shell32 ole32)
endif()

# Generic developer tools: neither target reads or embeds a ROM.
add_subdirectory("${ENGINE}/recompiler" "${CMAKE_CURRENT_BINARY_DIR}/recompiler")
target_sources(NESRecomp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/utf8.manifest")
add_executable(ArcadePrepare prepare_main.cpp utf8.manifest)
target_compile_features(ArcadePrepare PRIVATE cxx_std_17)
target_compile_options(ArcadePrepare PRIVATE /utf-8 /W4)
target_compile_definitions(ArcadePrepare PRIVATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)
target_link_libraries(ArcadePrepare PRIVATE bcrypt)

if(ARCADE_PREPARE_MODULE)
    set(ARCADE_ROM "${ARCADE_ROOT}/roms/Mario Bros..nes" CACHE FILEPATH "Private supported ROM")
    if(NOT EXISTS "${ARCADE_ROM}")
        message(FATAL_ERROR "Supply the private supported ROM with ARCADE_ROM.")
    endif()
    file(SHA256 "${ARCADE_ROM}" ROM_SHA256)
    if(NOT ROM_SHA256 STREQUAL "a2a0aa437a735cd36ed29a8e7c5b3dfa10590c6203e893f1668c7995bad2b309")
        message(FATAL_ERROR "The ROM is not the supported revision; no module was generated.")
    endif()
    find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
    set(ARCADE_RECOMPILER "${ENGINE}/build/recompiler/Release/NESRecomp.exe" CACHE FILEPATH "NESRecomp developer tool")
    set(GENERATED "${CMAKE_CURRENT_BINARY_DIR}/private-module")
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${ENGINE}/tools/cyc/prepare_project.py"
        --rom "${ARCADE_ROM}" --recompiler "${ARCADE_RECOMPILER}" --out "${GENERATED}"
        --game "${ARCADE_ROOT}/game.toml"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Private generation failed: ${output}\n${error}")
    endif()
    include("${GENERATED}/sources.cmake")
    set(SDK "${CMAKE_CURRENT_BINARY_DIR}/generic-sdk")
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_sdk.py" "${ENGINE}" "${SDK}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "SDK preparation failed: ${output}\n${error}")
    endif()

    add_library(ArcadeLocalModule MODULE ${CYC_PROJECT_SOURCES} module_entry.c)
    target_compile_features(ArcadeLocalModule PRIVATE c_std_11)
    target_compile_options(ArcadeLocalModule PRIVATE /utf-8 /GS-)
    target_include_directories(ArcadeLocalModule PRIVATE "${SDK}/runner/cyc" "${SDK}/runner/include" "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(ArcadeLocalModule PRIVATE ArcadeNativeHost)
    target_link_options(ArcadeLocalModule PRIVATE /NOENTRY /NODEFAULTLIB)

    add_executable(ArcadeNativeReference $<TARGET_OBJECTS:ArcadeMachine> ${CYC_PROJECT_SOURCES} reference_main.c)
    target_compile_features(ArcadeNativeReference PRIVATE c_std_11)
    target_compile_options(ArcadeNativeReference PRIVATE /utf-8)
    target_include_directories(ArcadeNativeReference PRIVATE ${NESRECOMP_CYC_INCLUDE_DIRS})
    target_link_libraries(ArcadeNativeReference PRIVATE ${NESRECOMP_CYC_LIBRARIES})

    add_library(ArcadeBadModule MODULE bad_module.c)
    target_compile_features(ArcadeBadModule PRIVATE c_std_11)
    target_compile_options(ArcadeBadModule PRIVATE /utf-8 /GS-)
    target_link_options(ArcadeBadModule PRIVATE /NOENTRY /NODEFAULTLIB)
endif()
