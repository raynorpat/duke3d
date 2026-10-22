# Drives the SDL2 video and timer layer of the Build engine without game
# data. Runs headless via SDL's dummy video driver.
add_executable(sdl_driver_smoke sdl_driver_smoke.c)
target_link_libraries(sdl_driver_smoke PRIVATE buildengine duke3d::SDL2main)

add_test(NAME sdl_driver_smoke COMMAND sdl_driver_smoke)
set_tests_properties(sdl_driver_smoke PROPERTIES
  ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
  TIMEOUT 30)

add_test(NAME sdl_driver_smoke_gl_fallback COMMAND sdl_driver_smoke gl_fallback)
add_test(NAME sdl_driver_smoke_displaymode_setter COMMAND sdl_driver_smoke displaymode_setter)
add_test(NAME sdl_driver_smoke_mode_sizes COMMAND sdl_driver_smoke mode_sizes)
add_test(NAME sdl_driver_smoke_palette_fade COMMAND sdl_driver_smoke palette_fade)
add_test(NAME sdl_driver_smoke_screencapture COMMAND sdl_driver_smoke screencapture_formats)
add_test(NAME sdl_driver_smoke_video_fade COMMAND sdl_driver_smoke video_fade)
set_tests_properties(sdl_driver_smoke_gl_fallback sdl_driver_smoke_displaymode_setter
                     sdl_driver_smoke_mode_sizes
                     sdl_driver_smoke_palette_fade
                     sdl_driver_smoke_screencapture
                     sdl_driver_smoke_video_fade PROPERTIES
  ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
  TIMEOUT 30)

# Checks DUKE3D_GAMEVER, BUILD_ENGINEREV, BYTEVERSION and the volume macros
# against this configuration's DUKE3D_VERSION. The table is kept here, apart
# from the one that sets them, so a mistake in either shows up.
set(_vexp_1.0_sw  199601290 19960129 10  0)
set(_vexp_1.1_sw  199602200 19960220 12  0)
set(_vexp_1.3D    199604190 19960419 103 0)
set(_vexp_1.3D_sw 199604190 19960419 14  0)
set(_vexp_1.4     199610210 19960820 116 1)
set(_vexp_1.5     199610320 20000614 117 1)
set(_vexp_1.5_sw  199610320 20000614 27  0)
set(_vkey "${DUKE3D_VERSION}")
if(DUKE3D_SHAREWARE)
  string(APPEND _vkey "_sw")
endif()
list(GET _vexp_${_vkey} 0 _vgame)
list(GET _vexp_${_vkey} 1 _veng)
list(GET _vexp_${_vkey} 2 _vbyte)
list(GET _vexp_${_vkey} 3 _vpluto)
if(DUKE3D_SHAREWARE)
  set(_vone 1)
else()
  set(_vone 0)
endif()
add_executable(version_macros version_macros.c)
# Only the headers are needed, so it takes duke3d_game's include path and
# options rather than its objects.
target_include_directories(version_macros PRIVATE "${PROJECT_SOURCE_DIR}/source")
target_link_libraries(version_macros PRIVATE
  duke3d_game_options buildengine audiolib duke3d::SDL2main)
target_compile_definitions(version_macros PRIVATE
  SDL_MAIN_HANDLED EXPECT_GAMEVER=${_vgame}L EXPECT_ENGINEREV=${_veng}L
  EXPECT_BYTEVERSION=${_vbyte} EXPECT_PLUTOPAK=${_vpluto}
  EXPECT_VOLUMEONE=${_vone})
add_test(NAME version_macros COMMAND version_macros)

# The engine's C fixed-point helpers against the original x86 results.
add_executable(pragmas_semantics pragmas_semantics.c)
target_include_directories(pragmas_semantics PRIVATE "${PROJECT_SOURCE_DIR}/source")
target_link_libraries(pragmas_semantics PRIVATE buildengine)
add_test(NAME pragmas_semantics COMMAND pragmas_semantics)

# Renderer 3's draw path from what the driver has, and the setting's names.
# Built without the engine (and OpenGL), as the settings readers use it.
add_executable(gl_path gl_path.c "${PROJECT_SOURCE_DIR}/source/build/glext.c")
target_include_directories(gl_path PRIVATE "${PROJECT_SOURCE_DIR}/source/build")
target_link_libraries(gl_path PRIVATE duke3d_compile_options)
add_test(NAME gl_path COMMAND gl_path)

# inside() per engine revision.
add_executable(engine_inside engine_inside.c)
target_include_directories(engine_inside PRIVATE "${PROJECT_SOURCE_DIR}/source")
target_link_libraries(engine_inside PRIVATE buildengine)
add_test(NAME engine_inside COMMAND engine_inside)

add_executable(engine_trace engine_trace.c)
target_include_directories(engine_trace PRIVATE "${PROJECT_SOURCE_DIR}/source")
target_link_libraries(engine_trace PRIVATE buildengine)
add_test(NAME engine_trace COMMAND engine_trace)

add_executable(vline_counts vline_counts.c)
target_link_libraries(vline_counts PRIVATE buildengine)
add_test(NAME vline_counts COMMAND vline_counts)

# The engine's LZW compressor and decompressor must agree. In 1.3D builds
# with the 1.3D data, it also reads a save written by the DOS executable.
add_executable(lzw_roundtrip lzw_roundtrip.c)
target_include_directories(lzw_roundtrip PRIVATE "${PROJECT_SOURCE_DIR}/source")
target_link_libraries(lzw_roundtrip PRIVATE buildengine)
add_test(NAME lzw_roundtrip COMMAND lzw_roundtrip
         WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# Plays a MIDI song through the FM driver and the emulated OPL3 without an
# audio device, and checks the note sounds and keeps the song's tempo.
add_executable(opl_music opl_music.c)
target_include_directories(opl_music PRIVATE "${PROJECT_SOURCE_DIR}/source")
target_link_libraries(opl_music PRIVATE audiolib)
add_test(NAME opl_music COMMAND opl_music)
set_tests_properties(opl_music PROPERTIES TIMEOUT 30)

# Stops audio playback with "interrupts" disabled, as MV_Shutdown does,
# which must not deadlock against the audio callback. Uses SDL's dummy
# audio driver, so it needs no sound hardware.
add_executable(dsl_shutdown dsl_shutdown.c)
target_include_directories(dsl_shutdown PRIVATE "${PROJECT_SOURCE_DIR}/source")
target_link_libraries(dsl_shutdown PRIVATE audiolib duke3d::SDL2main)
target_compile_definitions(dsl_shutdown PRIVATE SDL_MAIN_HANDLED)
add_test(NAME dsl_shutdown COMMAND dsl_shutdown)
set_tests_properties(dsl_shutdown PROPERTIES
  ENVIRONMENT "SDL_AUDIODRIVER=dummy"
  TIMEOUT 10)

# Bad DUKE3D_VERSION values must stop the configure step.
add_test(NAME version_config_rejects
  COMMAND "${CMAKE_COMMAND}" "-DSRC=${PROJECT_SOURCE_DIR}"
          "-DWORK=${CMAKE_BINARY_DIR}/vcr" -P
          "${CMAKE_CURRENT_SOURCE_DIR}/version_config_rejects.cmake")
set_tests_properties(version_config_rejects PROPERTIES TIMEOUT 120)

# Windows: config and saves go to Saved Games\Duke Nukem 3D, and existing
# ones in the game folder are copied there. Needs no game data.
if(WIN32)
  add_test(NAME save_dir
    COMMAND "${CMAKE_COMMAND}"
            "-DDUKE3D=$<TARGET_FILE:duke3d>"
            "-DEMPTY_GRP=${CMAKE_CURRENT_SOURCE_DIR}/data/empty.grp"
            "-DWORK=${CMAKE_BINARY_DIR}/sd"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/save_dir.cmake")
  set_tests_properties(save_dir PROPERTIES TIMEOUT 60)
endif()

# Renders E1L1's start view from a real GRP and compares the frame
# checksum. Needs game data, so it is only registered when DUKE3D_GRP
# names an existing file.
add_executable(render_e1l1 render_e1l1.c)
target_link_libraries(render_e1l1 PRIVATE buildengine duke3d::SDL2main)

add_executable(render_classic2d render_classic2d.c)
target_link_libraries(render_classic2d PRIVATE buildengine duke3d::SDL2main)

add_executable(render_maskorder render_maskorder.c)
target_link_libraries(render_maskorder PRIVATE buildengine duke3d::SDL2main)

set(DUKE3D_GRP "" CACHE FILEPATH "DUKE3D.GRP for tests that need game data")
set(DUKE3D_E1L1_CHECKSUM "e0517697" CACHE STRING
    "Expected render_e1l1 frame checksum (hex); empty just prints it")
set(DUKE3D_E1L1_TILT_CHECKSUM "64e7f538" CACHE STRING
    "Expected render_e1l1_tilt (screen tilt and low detail at 1600x900) checksum (hex); empty just prints it")
# arm64 macOS gives 0d60cca6 in 1.3D builds and d64ef539 in 1.4/1.5 ones
# (Polymost takes slope heights from getzsofslope, whose maths changed
# between engine revisions). Not the default: Polymost is floating point, so
# other platforms and compilers may round differently.
set(DUKE3D_E1L1_POLYMOST_CHECKSUM "" CACHE STRING
    "Expected render_e1l1_polymost (renderer 2) frame checksum (hex); empty just prints it")
# render_classic2d's HUD and map view checksums (hex) for the 1.3D GRP;
# empty just prints them. They are the same in 1.3D and 1.5 builds. The
# wide modes are pinned with the widescreen port; tall modes must stay as
# they were.
foreach(_pin "320x200;9f03fd16;763b8c56" "640x400;c6b6b632;7282abe5"
             "640x480;92082500;dfe0efbe" "1280x1024;43229c16;441cd127"
             "480x640;6a4e30ce;28839020" "1280x720;7507d7cd;cc44ab59"
             "1600x900;872652c4;cc8d7da2")
  list(GET _pin 0 _size)
  list(GET _pin 1 _hud)
  list(GET _pin 2 _map)
  set(DUKE3D_CLASSIC2D_${_size}_HUD "${_hud}" CACHE STRING
      "Expected render_classic2d_${_size} HUD checksum (hex); empty just prints it")
  set(DUKE3D_CLASSIC2D_${_size}_MAP "${_map}" CACHE STRING
      "Expected render_classic2d_${_size} map view checksum (hex); empty just prints it")
endforeach()
if(DUKE3D_GRP AND EXISTS "${DUKE3D_GRP}")
  add_test(NAME render_e1l1
    COMMAND render_e1l1 "${DUKE3D_GRP}"
            "${CMAKE_CURRENT_BINARY_DIR}/render_e1l1.bmp"
            ${DUKE3D_E1L1_CHECKSUM})
  set_tests_properties(render_e1l1 PROPERTIES
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
    TIMEOUT 30)
  # game.c's screen tilt and low detail in a widescreen mode whose tile is a
  # power of two wide.
  add_test(NAME render_e1l1_tilt
    COMMAND render_e1l1 "${DUKE3D_GRP}"
            "${CMAKE_CURRENT_BINARY_DIR}/render_e1l1_tilt.bmp"
            "${DUKE3D_E1L1_TILT_CHECKSUM}" 0 tilt)
  set_tests_properties(render_e1l1_tilt PROPERTIES
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
    TIMEOUT 30)

  # Views narrower than a Polymost one before them (which leaves every
  # column's bounds open) draw nothing right of their edge.
  add_test(NAME render_e1l1_narrow
    COMMAND render_e1l1 "${DUKE3D_GRP}"
            "${CMAKE_CURRENT_BINARY_DIR}/render_e1l1_narrow.bmp" "" 0 narrow)
  set_tests_properties(render_e1l1_narrow PROPERTIES
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
    TIMEOUT 30)

  # Classic renderer HUD rotatesprites and textured map view at several
  # 8-bit modes.
  foreach(_size 320x200 640x400 640x480 1280x1024 480x640 1280x720 1600x900)
    string(REPLACE "x" ";" _wh "${_size}")
    add_test(NAME render_classic2d_${_size}
      COMMAND render_classic2d "${DUKE3D_GRP}"
              "${CMAKE_CURRENT_BINARY_DIR}/render_classic2d_${_size}" ${_wh}
              "${DUKE3D_CLASSIC2D_${_size}_HUD}" "${DUKE3D_CLASSIC2D_${_size}_MAP}")
    set_tests_properties(render_classic2d_${_size} PROPERTIES
      ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
      TIMEOUT 30)
  endforeach()

  # The same view through Polymost: renderer 2 (software) and 1 (solid
  # colour). Polymost is floating point, so the checksum may differ by
  # platform; renderer 1 only prints its checksum.
  if(DUKE3D_POLYMOST)
    add_test(NAME render_e1l1_polymost
      COMMAND render_e1l1 "${DUKE3D_GRP}"
              "${CMAKE_CURRENT_BINARY_DIR}/render_e1l1_polymost.bmp"
              "${DUKE3D_E1L1_POLYMOST_CHECKSUM}" 2)
    add_test(NAME render_e1l1_polymost_solid
      COMMAND render_e1l1 "${DUKE3D_GRP}"
              "${CMAKE_CURRENT_BINARY_DIR}/render_e1l1_polymost_solid.bmp"
              "" 1)
    # Renderer 2 drawing a render-to-tile view with a power-of-two height.
    add_test(NAME render_e1l1_polymost_tile
      COMMAND render_e1l1 "${DUKE3D_GRP}"
              "${CMAKE_CURRENT_BINARY_DIR}/render_e1l1_polymost_tile.bmp"
              "" 2 tile)
    # The same tilt and low detail in renderer 2, against renderer 0's.
    add_test(NAME render_e1l1_polymost_tilt
      COMMAND render_e1l1 "${DUKE3D_GRP}"
              "${CMAKE_CURRENT_BINARY_DIR}/render_e1l1_polymost_tilt.bmp"
              "" 2 tilt)
    # A sprite behind a masked wall (JFBuild 61483f0). Renderer 1 first, so
    # renderer 0's xb1/xb2 aren't left to stand in for dxb1/dxb2.
    add_test(NAME render_maskorder
      COMMAND render_maskorder "${DUKE3D_GRP}" 1 2 0)
    set_tests_properties(render_e1l1_polymost render_e1l1_polymost_solid
                         render_e1l1_polymost_tile render_e1l1_polymost_tilt
                         render_maskorder
      PROPERTIES
      ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
      TIMEOUT 30)
  endif()

  # Renderer 3 in a real OpenGL window: 2D drawing, screencapture and a
  # fade, with images left in the build tree to check by eye. It opens a
  # window, so it's opt-in; with no OpenGL (the dummy driver) it's skipped.
  option(DUKE3D_GL_TESTS "Run the renderer 3 test, which opens a window" OFF)
  if(DUKE3D_POLYMOST AND DUKE3D_GL_TESTS)
    # The game is linked in for its renderer 3 palette code; game.c's main()
    # is renamed (below), as in con_compile.
    add_executable(render_gl render_gl.c render_gl_game.c
                   "${PROJECT_SOURCE_DIR}/source/game.c")
    target_link_libraries(render_gl PRIVATE duke3d_game duke3d::SDL2main)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/render_gl")
    add_test(NAME render_gl
      COMMAND render_gl "${DUKE3D_GRP}" "${CMAKE_CURRENT_BINARY_DIR}/render_gl")
    set_tests_properties(render_gl PROPERTIES
      ENVIRONMENT "BUILD_WINDOWED=1"
      SKIP_RETURN_CODE 77
      TIMEOUT 30)
    # The VIDEO page's live renderer switch, from a run with no GL context
    # yet, its modes, and the fade around them.
    add_test(NAME render_gl_video
      COMMAND render_gl "${DUKE3D_GRP}" "${CMAKE_CURRENT_BINARY_DIR}/render_gl" video)
    set_tests_properties(render_gl_video PROPERTIES
      ENVIRONMENT "BUILD_WINDOWED=1"
      SKIP_RETURN_CODE 77
      TIMEOUT 60)
    # Supersampling on the shader path: the frame shown is the large one
    # box-filtered down, its sizes, clamp and the settings that turn it off.
    add_test(NAME render_gl_supersample
      COMMAND render_gl "${DUKE3D_GRP}" "${CMAKE_CURRENT_BINARY_DIR}/render_gl" supersample)
    set_tests_properties(render_gl_supersample PROPERTIES
      ENVIRONMENT "BUILD_WINDOWED=1"
      SKIP_RETURN_CODE 77
      TIMEOUT 60)
    # Texture filtering on every draw path: close to nearest, translucency
    # and fades kept, nearest again byte for byte, with supersampling too.
    add_test(NAME render_gl_filter
      COMMAND render_gl "${DUKE3D_GRP}" "${CMAKE_CURRENT_BINARY_DIR}/render_gl" filter)
    set_tests_properties(render_gl_filter PROPERTIES
      ENVIRONMENT "BUILD_WINDOWED=1"
      SKIP_RETURN_CODE 77
      TIMEOUT 90)
  endif()

  # Boots the real game to its title loop and into E1L1, headless. It uses
  # the in-repo 1.5 CONs, so not in 1.3D builds.
  if(NOT DUKE3D_VERSION STREQUAL "1.3D")
    if(WIN32)
      set(_cfg_dir "Duke Nukem 3D")
    elseif(APPLE)
      set(_cfg_dir "Library/Application Support/Duke Nukem 3D")
    else()
      set(_cfg_dir ".duke3d")
    endif()
    add_test(NAME game_boot
      COMMAND "${CMAKE_COMMAND}"
              "-DDUKE3D=$<TARGET_FILE:duke3d>"
              "-DGRP=${DUKE3D_GRP}"
              "-DCON_DIR=${PROJECT_SOURCE_DIR}/testdata"
              "-DWORK=${CMAKE_BINARY_DIR}/gb"
              "-DCFG_DIR=${_cfg_dir}"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/game_boot.cmake")
    set_tests_properties(game_boot PROPERTIES TIMEOUT 60)
  endif()
endif()

# Compiles the in-repo CON scripts with the game's own compiler and checks
# the script addresses it stores. Needs no game data. game.c is built again
# here with its main() renamed, since the test brings its own.
set(_game_c "${PROJECT_SOURCE_DIR}/source/game.c")
add_executable(con_compile con_compile.c "${_game_c}")
set_source_files_properties("${_game_c}" PROPERTIES
  COMPILE_DEFINITIONS "SDL_MAIN_HANDLED;main=duke3d_main")
target_link_libraries(con_compile PRIVATE duke3d_game)

set(_con_dir "${CMAKE_CURRENT_BINARY_DIR}/con_compile")
foreach(_con game.con DEFS.CON USER.CON)
  string(TOLOWER "${_con}" _src)
  configure_file("${PROJECT_SOURCE_DIR}/testdata/${_src}" "${_con_dir}/${_con}"
                 COPYONLY)
endforeach()
# game.con with one action in front. The compiler can't nest includes, so
# this is a copy rather than an "include game.con".
file(READ "${PROJECT_SOURCE_DIR}/testdata/game.con" _game_con)
file(WRITE "${_con_dir}/prefix.con"
  "action CONCOMPILE_PREFIX 0 1 1 1 1\r\n${_game_con}")

# The in-repo CONs are 1.5 scripts, which a 1.3D, 1.1 or 1.0 build can't
# compile.
if(NOT DUKE3D_VERSION MATCHES "^(1\\.3D|1\\.1|1\\.0)$")
  add_test(NAME con_compile COMMAND con_compile WORKING_DIRECTORY "${_con_dir}")
  set_tests_properties(con_compile PROPERTIES TIMEOUT 30)
endif()

# Game helper functions against the original DOS libraries. Needs no data.
add_executable(game_helpers game_helpers.c "${_game_c}")
target_link_libraries(game_helpers PRIVATE duke3d_game)
add_test(NAME game_helpers COMMAND game_helpers)

# The small full-screen HUD (screen_size 4) at several 8-bit modes: in
# widescreen modes health sits at the screen's left edge and ammo and the
# inventory at its right; 4:3 and tall modes keep their layout (pinned
# checksums, hex, of the frame without and with an inventory item; empty
# just prints them). Needs game data.
add_executable(minihud minihud.c "${_game_c}")
target_link_libraries(minihud PRIVATE duke3d_game)
if(DUKE3D_GRP AND EXISTS "${DUKE3D_GRP}")
  foreach(_pin "320x200;f2070c6d;f191ac15" "640x480;3444f9f9;fab217a1"
               "1280x1024;b324dd41;8b7c4151" "480x640;6b424f6e;b6211dae"
               "1280x720;d08e6033;f49c6dcd" "1600x900;742b8451;ac001fed")
    list(GET _pin 0 _size)
    list(GET _pin 1 _plain)
    list(GET _pin 2 _inv)
    string(REPLACE "x" ";" _wh "${_size}")
    add_test(NAME minihud_${_size}
      COMMAND minihud "${DUKE3D_GRP}"
              "${CMAKE_CURRENT_BINARY_DIR}/minihud_${_size}" ${_wh}
              "${_plain}" "${_inv}")
    set_tests_properties(minihud_${_size} PROPERTIES
      ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
      TIMEOUT 30)
  endforeach()
endif()

# Checks the save record codec against the DOS 1.5 byte layout. Needs no
# game data.
add_executable(savegame_codec savegame_codec.c "${_game_c}")
target_link_libraries(savegame_codec PRIVATE duke3d_game)
add_test(NAME savegame_codec COMMAND savegame_codec)
set_tests_properties(savegame_codec PROPERTIES TIMEOUT 30)

# Loads original DOS 1.5 (Atomic Edition) saves. Needs the Atomic GRP and a
# folder of DOS saves, so the tests are only registered when both are set.
# tools/extract_iso_files.py copies the GRP and CONs off the Atomic CD.
set(DUKE3D_ATOMIC_GRP "" CACHE FILEPATH
    "Atomic Edition (1.5) DUKE3D.GRP for the DOS save tests")
set(DUKE3D_DOS_SAVES_DIR "" CACHE PATH
    "Folder searched for DOS 1.5 .sav files for the DOS save tests")
find_package(Python3 COMPONENTS Interpreter)
if(DUKE3D_VERSION STREQUAL "1.5" AND NOT DUKE3D_SHAREWARE AND
   DUKE3D_ATOMIC_GRP AND EXISTS "${DUKE3D_ATOMIC_GRP}" AND
   DUKE3D_DOS_SAVES_DIR AND IS_DIRECTORY "${DUKE3D_DOS_SAVES_DIR}" AND
   Python3_Interpreter_FOUND)
  file(GLOB_RECURSE _saves "${DUKE3D_DOS_SAVES_DIR}/*")
  list(FILTER _saves INCLUDE REGEX "\\.[sS][aA][vV]$")
  list(SORT _saves)

  set(_n 0)
  function(add_dos_save_test name mode save)
    math(EXPR _n "${_n} + 1")
    set(_n ${_n} PARENT_SCOPE)
    add_test(NAME ${name}
      COMMAND "${CMAKE_COMMAND}"
              "-DMODE=${mode}"
              "-DDUKE3D=$<TARGET_FILE:duke3d>"
              "-DGRP=${DUKE3D_ATOMIC_GRP}"
              "-DSAVE=${save}"
              "-DPYTHON=${Python3_EXECUTABLE}"
              "-DDECODER=${CMAKE_CURRENT_SOURCE_DIR}/dos_save_decode.py"
              "-DWORK=${CMAKE_BINARY_DIR}/ds${_n}"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/dos_save.cmake")
    set_tests_properties(${name} PROPERTIES TIMEOUT 180)
  endfunction()

  foreach(_save IN LISTS _saves)
    file(RELATIVE_PATH _rel "${DUKE3D_DOS_SAVES_DIR}" "${_save}")
    string(MAKE_C_IDENTIFIER "${_rel}" _id)
    add_dos_save_test(dos_save_load_${_id} load "${_save}")
  endforeach()

  # One round trip each from Episodes I, II and IV when the folders are
  # named that way; otherwise the first save.
  set(_rt "")
  foreach(_ep "Episode I/" "Episode II/" "Episode IV/")
    foreach(_save IN LISTS _saves)
      string(FIND "${_save}" "${_ep}" _at)
      if(NOT _at EQUAL -1)
        list(APPEND _rt "${_save}")
        break()
      endif()
    endforeach()
  endforeach()
  if(NOT _rt AND _saves)
    list(GET _saves 0 _rt)
  endif()
  foreach(_save IN LISTS _rt)
    file(RELATIVE_PATH _rel "${DUKE3D_DOS_SAVES_DIR}" "${_save}")
    string(MAKE_C_IDENTIFIER "${_rel}" _id)
    add_dos_save_test(dos_save_roundtrip_${_id} roundtrip "${_save}")
  endforeach()

  if(_saves)
    list(GET _saves 0 _first)
    add_dos_save_test(dos_save_refuse refuse "${_first}")
    add_dos_save_test(dos_save_hostile hostile "${_first}")
  endif()
endif()

# 1.3D builds: compile the original 1.3D CONs and compare the bytecode with a
# DOS save made by the original executable. Needs the 1.3D data, so the tests
# are only registered when its folder is set (docs/v13d/reference.md).
set(DUKE3D_V13D_DATA_DIR "" CACHE PATH "Duke Nukem 3D 1.3D full release folder")
set(DUKE3D_V13D_SW_DATA_DIR "" CACHE PATH "Duke Nukem 3D 1.3D shareware folder")
if(DUKE3D_VERSION STREQUAL "1.3D" AND Python3_Interpreter_FOUND)
  if(DUKE3D_SHAREWARE)
    set(_v13d_edition sw)
    set(_v13d_data "${DUKE3D_V13D_SW_DATA_DIR}")
  else()
    set(_v13d_edition full)
    set(_v13d_data "${DUKE3D_V13D_DATA_DIR}")
  endif()
  if(_v13d_data)
    add_test(NAME con_compile_v13d_${_v13d_edition}
      COMMAND "${CMAKE_COMMAND}"
              "-DDATA=${_v13d_data}" "-DEDITION=${_v13d_edition}"
              "-DCON_COMPILE=$<TARGET_FILE:con_compile>"
              "-DPYTHON=${Python3_EXECUTABLE}"
              "-DDECODER=${CMAKE_CURRENT_SOURCE_DIR}/v13d_save_decode.py"
              "-DWORK=${CMAKE_BINARY_DIR}/cc13"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/con_compile_v13d.cmake")
    set_tests_properties(con_compile_v13d_${_v13d_edition} PROPERTIES TIMEOUT 120)
  endif()
  if(_v13d_data)
    add_test(NAME game_boot_v13d_${_v13d_edition}
      COMMAND "${CMAKE_COMMAND}"
              "-DDUKE3D=$<TARGET_FILE:duke3d>"
              "-DDATA=${_v13d_data}" "-DEDITION=${_v13d_edition}"
              "-DWORK=${CMAKE_BINARY_DIR}/gb13"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/game_boot_v13d.cmake")
    set_tests_properties(game_boot_v13d_${_v13d_edition} PROPERTIES TIMEOUT 240)
  endif()
  if(_v13d_data)
    # The original demos, played to the end and compared with the DOS state.
    if(_v13d_edition STREQUAL "full")
      set(_v13d_demos 1 2)
    else()
      set(_v13d_demos 1 2 3)
    endif()
    foreach(_demo IN LISTS _v13d_demos)
      add_test(NAME demo_sync_v13d_${_v13d_edition}_${_demo}
        COMMAND "${CMAKE_COMMAND}"
                "-DDUKE3D=$<TARGET_FILE:duke3d>"
                "-DDATA=${_v13d_data}" "-DEDITION=${_v13d_edition}"
                "-DDEMO=${_demo}"
                "-DREFS=${CMAKE_CURRENT_SOURCE_DIR}/data/v13d_demo_refs.cmake"
                "-DWORK=${CMAKE_BINARY_DIR}/ds13_${_demo}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/demo_sync_v13d.cmake")
      set_tests_properties(demo_sync_v13d_${_v13d_edition}_${_demo} PROPERTIES TIMEOUT 400)
    endforeach()
    # Demo 1 again, drawn, with the renderer and mode switched every 100 tics
    # as the VIDEO page does, each faded and waited for (OpenGL fails
    # headless, and stays off): still in sync. The demo stands still during
    # each switch; net_videocycle switches while play goes on.
    add_test(NAME demo_switch_v13d_${_v13d_edition}
      COMMAND "${CMAKE_COMMAND}"
              "-DDUKE3D=$<TARGET_FILE:duke3d>"
              "-DDATA=${_v13d_data}" "-DEDITION=${_v13d_edition}"
              -DDEMO=1 "-DFLAGS=-demoswitch 100"
              "-DREFS=${CMAKE_CURRENT_SOURCE_DIR}/data/v13d_demo_refs.cmake"
              "-DWORK=${CMAKE_BINARY_DIR}/dsw13"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/demo_sync_v13d.cmake")
    set_tests_properties(demo_switch_v13d_${_v13d_edition} PROPERTIES TIMEOUT 400)
    add_test(NAME demo_refuse_v13d_${_v13d_edition}
      COMMAND "${CMAKE_COMMAND}"
              "-DDUKE3D=$<TARGET_FILE:duke3d>"
              "-DDATA=${_v13d_data}" "-DEDITION=${_v13d_edition}"
              -DDEMO=1 -DMODE=refuse
              "-DWORK=${CMAKE_BINARY_DIR}/dr13"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/demo_sync_v13d.cmake")
    set_tests_properties(demo_refuse_v13d_${_v13d_edition} PROPERTIES TIMEOUT 120)
    if(_v13d_edition STREQUAL "sw")
      add_test(NAME demo_fullonly_v13d_sw
        COMMAND "${CMAKE_COMMAND}"
                "-DDUKE3D=$<TARGET_FILE:duke3d>"
                "-DDATA=${_v13d_data}" "-DEDITION=${_v13d_edition}"
                -DDEMO=1 -DMODE=fullonly
                "-DWORK=${CMAKE_BINARY_DIR}/df13"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/demo_sync_v13d.cmake")
      set_tests_properties(demo_fullonly_v13d_sw PROPERTIES TIMEOUT 120)
    endif()
  endif()
  if(_v13d_data AND EXISTS "${_v13d_data}/refsaves/e1l1start.SAV")
    # Its own directory: lzw_roundtrip writes a scratch file in its working
    # directory, and the plain lzw_roundtrip test may run at the same time.
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lzw_dos")
    add_test(NAME lzw_dos_v13d_${_v13d_edition}
      COMMAND lzw_roundtrip "${_v13d_data}/refsaves/e1l1start.SAV"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lzw_dos")
    # Each DOS reference save of this edition must load with the state its status
    # bar showed, and survive our own save and reload.
    include("${CMAKE_CURRENT_SOURCE_DIR}/data/v13d_save_refs.cmake")
    get_cmake_property(_v13d_vars VARIABLES)
    foreach(_var IN LISTS _v13d_vars)
      if(_var MATCHES "^V13D_SAVE_(.+)_EDITION$" AND "${${_var}}" STREQUAL _v13d_edition)
        set(_name "${CMAKE_MATCH_1}")
        add_test(NAME dos_save_v13d_${_name}
          COMMAND "${CMAKE_COMMAND}" -DMODE=load
                  "-DDUKE3D=$<TARGET_FILE:duke3d>"
                  "-DDATA=${_v13d_data}" "-DEDITION=${_v13d_edition}"
                  "-DREFS=${CMAKE_CURRENT_SOURCE_DIR}/data/v13d_save_refs.cmake"
                  "-DPYTHON=${Python3_EXECUTABLE}"
                  "-DDECODER=${CMAKE_CURRENT_SOURCE_DIR}/v13d_save_decode.py"
                  "-DNAME=${_name}" "-DWORK=${CMAKE_BINARY_DIR}/sv13_${_name}"
                  -P "${CMAKE_CURRENT_SOURCE_DIR}/dos_save_v13d.cmake")
        set_tests_properties(dos_save_v13d_${_name} PROPERTIES TIMEOUT 120)
      endif()
    endforeach()
    # The other edition's DOS save, and a 1.5 one, must be refused.
    if(_v13d_edition STREQUAL "full")
      set(_v13d_other "${DUKE3D_V13D_SW_DATA_DIR}/refsaves/e1l1start.SAV")
    else()
      set(_v13d_other "${DUKE3D_V13D_DATA_DIR}/refsaves/e1l1start.SAV")
    endif()
    if(EXISTS "${_v13d_other}")
      add_test(NAME save_refuse_v13d_${_v13d_edition}
        COMMAND "${CMAKE_COMMAND}" -DMODE=refuse
                "-DDUKE3D=$<TARGET_FILE:duke3d>"
                "-DDATA=${_v13d_data}" "-DEDITION=${_v13d_edition}"
                "-DOTHER=${_v13d_other}" "-DWORK=${CMAKE_BINARY_DIR}/sr13"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/dos_save_v13d.cmake")
      set_tests_properties(save_refuse_v13d_${_v13d_edition} PROPERTIES TIMEOUT 120)
    endif()
  endif()
  if(_v13d_data)
    # Two copies of this build must meet over UDP on localhost, and both
    # peers must refuse any other version or edition. Each DUKE3D_NET_PEERS
    # entry is "<duke3d executable>=<folder with its GRP and CONs>".
    set(DUKE3D_NET_PEERS "" CACHE STRING "Other duke3d builds to test multiplayer refusal against")
    if(_v13d_edition STREQUAL "full")
      set(_net_port 23600)
    else()
      set(_net_port 23700)
    endif()
    add_test(NAME net_connect_v13d_${_v13d_edition}
      COMMAND "${CMAKE_COMMAND}"
              "-DA=$<TARGET_FILE:duke3d>" "-DA_DATA=${_v13d_data}"
              "-DB=$<TARGET_FILE:duke3d>" "-DB_DATA=${_v13d_data}"
              -DPORT=${_net_port} -DMODE=connect "-DARGS=/v1 /l1"
              "-DWORK=${CMAKE_BINARY_DIR}/nc13"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/net_v13d.cmake")
    set_tests_properties(net_connect_v13d_${_v13d_edition} PROPERTIES TIMEOUT 90)
    # One peer changes renderer and resolution as the VIDEO page does, every
    # 20 frames (OpenGL fails headless; BUILD_WINDOWED keeps both windowed):
    # still in sync.
    math(EXPR _net_cycle_port "${_net_port} + 50")
    add_test(NAME net_videocycle_v13d_${_v13d_edition}
      COMMAND "${CMAKE_COMMAND}"
              "-DA=$<TARGET_FILE:duke3d>" "-DA_DATA=${_v13d_data}"
              "-DB=$<TARGET_FILE:duke3d>" "-DB_DATA=${_v13d_data}"
              -DPORT=${_net_cycle_port} -DMODE=connect "-DARGS=/v1 /l1"
              "-DA_ARGS=-videocycle 20" "-DA_EXPECT=VIDEOCYCLE renderer [0-9] -> [12].*drawn at 1600x900"
              "-DWORK=${CMAKE_BINARY_DIR}/nv13"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/net_v13d.cmake")
    set_tests_properties(net_videocycle_v13d_${_v13d_edition} PROPERTIES TIMEOUT 90)
    set(_net_n 0)
    foreach(_peer IN LISTS DUKE3D_NET_PEERS)
      string(REGEX REPLACE "=.*" "" _peer_exe "${_peer}")
      string(REGEX REPLACE "^[^=]*=" "" _peer_data "${_peer}")
      math(EXPR _net_n "${_net_n} + 1")
      math(EXPR _net_port "${_net_port} + 2")
      add_test(NAME net_refuse_v13d_${_v13d_edition}_${_net_n}
        COMMAND "${CMAKE_COMMAND}"
                "-DA=$<TARGET_FILE:duke3d>" "-DA_DATA=${_v13d_data}"
                "-DB=${_peer_exe}" "-DB_DATA=${_peer_data}"
                -DPORT=${_net_port} -DMODE=refuse
                "-DWORK=${CMAKE_BINARY_DIR}/nr13_${_net_n}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/net_v13d.cmake")
      set_tests_properties(net_refuse_v13d_${_v13d_edition}_${_net_n} PROPERTIES TIMEOUT 90)
    endforeach()
  endif()
  # The edition check must name a wrong edition, not fail some other way.
  if(DUKE3D_V13D_DATA_DIR)
    add_test(NAME v13d_edition_mismatch
      COMMAND "${CMAKE_COMMAND}" "-DDIR=${DUKE3D_V13D_DATA_DIR}" -DEDITION=sw
              -P "${CMAKE_CURRENT_SOURCE_DIR}/v13d_check.cmake")
    set_tests_properties(v13d_edition_mismatch PROPERTIES
      PASS_REGULAR_EXPRESSION "wrong edition: expected sw")
  endif()
endif()

# 1.1 and 1.0 shareware builds: the same scripts as the 1.3D tests, with the
# editions sw11 and sw10 (docs/v10-v11/reference.md).
set(DUKE3D_V11_SW_DATA_DIR "" CACHE PATH "Duke Nukem 3D 1.1 shareware folder")
set(DUKE3D_V10_SW_DATA_DIR "" CACHE PATH "Duke Nukem 3D 1.0 shareware folder")
if(DUKE3D_VERSION MATCHES "^1\\.[01]$" AND DUKE3D_SHAREWARE AND Python3_Interpreter_FOUND)
  if(DUKE3D_VERSION STREQUAL "1.1")
    set(_vold_edition sw11)
    set(_vold_name v11_sw)
    set(_vold_data "${DUKE3D_V11_SW_DATA_DIR}")
  else()
    set(_vold_edition sw10)
    set(_vold_name v10_sw)
    set(_vold_data "${DUKE3D_V10_SW_DATA_DIR}")
  endif()
  if(_vold_data)
    add_test(NAME con_compile_${_vold_name}
      COMMAND "${CMAKE_COMMAND}"
              "-DDATA=${_vold_data}" "-DEDITION=${_vold_edition}"
              "-DCON_COMPILE=$<TARGET_FILE:con_compile>"
              "-DPYTHON=${Python3_EXECUTABLE}"
              "-DDECODER=${CMAKE_CURRENT_SOURCE_DIR}/v13d_save_decode.py"
              "-DWORK=${CMAKE_BINARY_DIR}/cc_${_vold_name}"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/con_compile_v13d.cmake")
    set_tests_properties(con_compile_${_vold_name} PROPERTIES TIMEOUT 120)
    add_test(NAME game_boot_${_vold_name}
      COMMAND "${CMAKE_COMMAND}" "-DDUKE3D=$<TARGET_FILE:duke3d>"
              "-DDATA=${_vold_data}" "-DEDITION=${_vold_edition}"
              "-DWORK=${CMAKE_BINARY_DIR}/gb_${_vold_name}"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/game_boot_v13d.cmake")
    set_tests_properties(game_boot_${_vold_name} PROPERTIES TIMEOUT 240)
  endif()
  # The original demos, played to the end and compared with the DOS state.
  if(_vold_data)
    foreach(_demo 1 2 3)
      add_test(NAME demo_sync_${_vold_name}_${_demo}
        COMMAND "${CMAKE_COMMAND}" "-DDUKE3D=$<TARGET_FILE:duke3d>"
                "-DDATA=${_vold_data}" "-DEDITION=${_vold_edition}"
                "-DDEMO=${_demo}"
                "-DREFS=${CMAKE_CURRENT_SOURCE_DIR}/data/v13d_demo_refs.cmake"
                "-DWORK=${CMAKE_BINARY_DIR}/ds_${_vold_name}_${_demo}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/demo_sync_v13d.cmake")
      set_tests_properties(demo_sync_${_vold_name}_${_demo} PROPERTIES TIMEOUT 400)
    endforeach()
    # A 1.5 demo, and the 1.3D shareware one when that folder is set, must be
    # refused. 1.0 and 1.1 demos can't be told apart (F11-001, F11-007).
    set(_vold_others "")
    if(DUKE3D_V13D_SW_DATA_DIR)
      set(_vold_others "${DUKE3D_V13D_SW_DATA_DIR}/DEMO1.DMO")
    endif()
    add_test(NAME demo_refuse_${_vold_name}
      COMMAND "${CMAKE_COMMAND}" "-DDUKE3D=$<TARGET_FILE:duke3d>"
              "-DDATA=${_vold_data}" "-DEDITION=${_vold_edition}"
              -DDEMO=1 -DMODE=refuse "-DOTHERS=${_vold_others}"
              "-DWORK=${CMAKE_BINARY_DIR}/dr_${_vold_name}"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/demo_sync_v13d.cmake")
    set_tests_properties(demo_refuse_${_vold_name} PROPERTIES TIMEOUT 120)
  endif()
  # Each DOS reference save of this release must load with the state its
  # status bar showed, and survive our own save and reload.
  if(_vold_data)
    include("${CMAKE_CURRENT_SOURCE_DIR}/data/v13d_save_refs.cmake")
    get_cmake_property(_vars VARIABLES)
    foreach(_var IN LISTS _vars)
      if(_var MATCHES "^V13D_SAVE_(.+)_EDITION$" AND "${${_var}}" STREQUAL _vold_edition)
        set(_ref "${CMAKE_MATCH_1}")
        string(REGEX REPLACE "^sw1[01]_" "" _name "${_ref}")
        add_test(NAME dos_save_${_vold_name}_${_name}
          COMMAND "${CMAKE_COMMAND}" -DMODE=load "-DNAME=${_ref}"
                  "-DDUKE3D=$<TARGET_FILE:duke3d>"
                  "-DDATA=${_vold_data}" "-DEDITION=${_vold_edition}"
                  "-DREFS=${CMAKE_CURRENT_SOURCE_DIR}/data/v13d_save_refs.cmake"
                  "-DPYTHON=${Python3_EXECUTABLE}"
                  "-DDECODER=${CMAKE_CURRENT_SOURCE_DIR}/v13d_save_decode.py"
                  "-DWORK=${CMAKE_BINARY_DIR}/sv_${_name}"
                  -P "${CMAKE_CURRENT_SOURCE_DIR}/dos_save_v13d.cmake")
        set_tests_properties(dos_save_${_vold_name}_${_name} PROPERTIES TIMEOUT 120)
      endif()
    endforeach()
    # The other release's and the 1.3D shareware saves, when their folders are
    # set, and a 1.5 one must be refused (rulings.md: 1.1's BYTEVERSION 12
    # collides with neither; 1.0 has none and goes by its layout).
    if(_vold_edition STREQUAL "sw11")
      set(_vold_other_dir "${DUKE3D_V10_SW_DATA_DIR}")
    else()
      set(_vold_other_dir "${DUKE3D_V11_SW_DATA_DIR}")
    endif()
    set(_vold_others "")
    foreach(_other "${_vold_other_dir}" "${DUKE3D_V13D_SW_DATA_DIR}")
      if(_other AND EXISTS "${_other}/refsaves/e1l1start.SAV")
        if(_vold_others)
          string(APPEND _vold_others "$<SEMICOLON>")
        endif()
        string(APPEND _vold_others "${_other}/refsaves/e1l1start.SAV")
      endif()
    endforeach()
    add_test(NAME save_refuse_${_vold_name}
      COMMAND "${CMAKE_COMMAND}" -DMODE=refuse
              "-DDUKE3D=$<TARGET_FILE:duke3d>"
              "-DDATA=${_vold_data}" "-DEDITION=${_vold_edition}"
              "-DOTHER=${_vold_others}" "-DWORK=${CMAKE_BINARY_DIR}/sr_${_vold_name}"
              "-DPYTHON=${Python3_EXECUTABLE}"
              "-DDECODER=${CMAKE_CURRENT_SOURCE_DIR}/v13d_save_decode.py"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/dos_save_v13d.cmake")
    set_tests_properties(save_refuse_${_vold_name} PROPERTIES TIMEOUT 120)
  endif()
  # Two copies of the 1.1 build must meet over UDP on localhost, and both
  # peers must refuse each DUKE3D_NET_PEERS build (1.0, 1.3D sw, ...) by
  # BYTEVERSION alone (rulings.md). The same for the 1.0 build, on its own ports.
  if(_vold_data)
    set(DUKE3D_NET_PEERS "" CACHE STRING "Other duke3d builds to test multiplayer refusal against")
    # DukeMatch (the default) and co-op (/c), both put straight into E1L1.
    if(_vold_edition STREQUAL "sw11")
      set(_net_port 23800)
    else()
      set(_net_port 23900)
    endif()
    foreach(_mode dm coop)
      if(_mode STREQUAL "dm")
        set(_net_args "/v1 /l1")
        set(_net_test net_connect_${_vold_name})
      else()
        set(_net_args "/v1 /l1 /c")
        set(_net_test net_connect_${_vold_name}_coop)
      endif()
      add_test(NAME ${_net_test}
        COMMAND "${CMAKE_COMMAND}"
                "-DA=$<TARGET_FILE:duke3d>" "-DA_DATA=${_vold_data}"
                "-DB=$<TARGET_FILE:duke3d>" "-DB_DATA=${_vold_data}"
                -DPORT=${_net_port} -DMODE=connect "-DARGS=${_net_args}"
                "-DWORK=${CMAKE_BINARY_DIR}/nc_${_vold_name}_${_mode}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/net_v13d.cmake")
      set_tests_properties(${_net_test} PROPERTIES TIMEOUT 90)
      math(EXPR _net_port "${_net_port} + 2")
    endforeach()
    set(_net_n 0)
    foreach(_peer IN LISTS DUKE3D_NET_PEERS)
      string(REGEX REPLACE "=.*" "" _peer_exe "${_peer}")
      string(REGEX REPLACE "^[^=]*=" "" _peer_data "${_peer}")
      math(EXPR _net_n "${_net_n} + 1")
      math(EXPR _net_port "${_net_port} + 2")
      add_test(NAME net_refuse_${_vold_name}_${_net_n}
        COMMAND "${CMAKE_COMMAND}"
                "-DA=$<TARGET_FILE:duke3d>" "-DA_DATA=${_vold_data}"
                "-DB=${_peer_exe}" "-DB_DATA=${_peer_data}"
                -DPORT=${_net_port} -DMODE=refuse
                "-DWORK=${CMAKE_BINARY_DIR}/nr_${_vold_name}_${_net_n}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/net_v13d.cmake")
      set_tests_properties(net_refuse_${_vold_name}_${_net_n} PROPERTIES TIMEOUT 90)
    endforeach()
  endif()
  # The edition check must refuse the 1.0 and 1.3D shareware as 1.1.
  if(_vold_edition STREQUAL "sw11" AND DUKE3D_V10_SW_DATA_DIR AND DUKE3D_V13D_SW_DATA_DIR)
    add_test(NAME v11_edition_mismatch
      COMMAND "${CMAKE_COMMAND}" -DEDITION=sw11
              "-DOTHERS=${DUKE3D_V10_SW_DATA_DIR}$<SEMICOLON>${DUKE3D_V13D_SW_DATA_DIR}"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/v13d_check.cmake")
  endif()
  # And the 1.1 and 1.3D shareware as 1.0.
  if(_vold_edition STREQUAL "sw10" AND DUKE3D_V11_SW_DATA_DIR AND DUKE3D_V13D_SW_DATA_DIR)
    add_test(NAME v10_edition_mismatch
      COMMAND "${CMAKE_COMMAND}" -DEDITION=sw10
              "-DOTHERS=${DUKE3D_V11_SW_DATA_DIR}$<SEMICOLON>${DUKE3D_V13D_SW_DATA_DIR}"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/v13d_check.cmake")
  endif()
endif()

# The editor build.cfg reader and writer; engine-free, so it runs everywhere.
add_executable(build_cfg build_cfg.c "${PROJECT_SOURCE_DIR}/source/buildcfg.c"
  "${PROJECT_SOURCE_DIR}/source/videoopts.c")
target_include_directories(build_cfg PRIVATE "${PROJECT_SOURCE_DIR}/source")
add_test(NAME build_cfg COMMAND build_cfg)

# The editor's 3D mode helper: renderer 3 falls back to classic without GL.
add_executable(editor_mode editor_mode.c "${PROJECT_SOURCE_DIR}/source/editormode.c")
target_include_directories(editor_mode PRIVATE "${PROJECT_SOURCE_DIR}/source")
target_link_libraries(editor_mode PRIVATE buildengine duke3d::SDL2main)
add_test(NAME editor_mode COMMAND editor_mode)
set_tests_properties(editor_mode PROPERTIES
  ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
  TIMEOUT 30)

# The VIDEO page's resolution list, draw size, supersampling and save
# tracking; engine-free.
add_executable(video_opts video_opts.c "${PROJECT_SOURCE_DIR}/source/videoopts.c")
target_include_directories(video_opts PRIVATE "${PROJECT_SOURCE_DIR}/source")
add_test(NAME video_opts COMMAND video_opts)
