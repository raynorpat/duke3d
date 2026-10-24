find_package(Python3 COMPONENTS Interpreter)

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

add_executable(net_protocol_peer net_protocol_peer.c)
target_link_libraries(net_protocol_peer PRIVATE buildengine duke3d::SDL2main)

function(add_protocol_refusal_tests name data port own_byte)
  # Other releases need no sibling build or private data folder. Optional
  # DUKE3D_NET_PEERS entries still exercise complete external game builds.
  foreach(_byteversion 9 10 12 14 103 116 117 27)
    if(_byteversion EQUAL own_byte)
      continue()
    endif()
    math(EXPR port "${port} + 2")
    add_test(NAME net_refuse_${name}_protocol_${_byteversion}
      COMMAND "${CMAKE_COMMAND}" "-DA=$<TARGET_FILE:net_game>" "-DA_DATA=${data}"
        "-DB=$<TARGET_FILE:net_protocol_peer>" -DB_VERSION=${_byteversion}
        -DPORT=${port} -DMODE=refuse "-DWORK=${CMAKE_BINARY_DIR}/np_${name}_${_byteversion}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/net_v13d.cmake")
    set_tests_properties(net_refuse_${name}_protocol_${_byteversion} PROPERTIES TIMEOUT 90)
  endforeach()
endfunction()

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

# Reinitializes the mixer and plays a sample in mono/8-bit and stereo/16-bit.
add_executable(multivoc_lifecycle multivoc_lifecycle.c)
target_include_directories(multivoc_lifecycle PRIVATE "${PROJECT_SOURCE_DIR}/source")
target_link_libraries(multivoc_lifecycle PRIVATE audiolib duke3d::SDL2main)
target_compile_definitions(multivoc_lifecycle PRIVATE SDL_MAIN_HANDLED)
add_test(NAME multivoc_lifecycle COMMAND multivoc_lifecycle)
set_tests_properties(multivoc_lifecycle PROPERTIES
  ENVIRONMENT "SDL_AUDIODRIVER=dummy"
  TIMEOUT 10)

# Reuse dependency sources/hints in isolated configure tests, including fetched SDL.
set(_configure_test_args "-DDUKE3D_FETCH_SDL=${DUKE3D_FETCH_SDL}")
if(DUKE3D_SDL2_FETCHED)
  list(APPEND _configure_test_args "-DFETCHCONTENT_SOURCE_DIR_SDL2=${sdl2_SOURCE_DIR}")
elseif(SDL2_DIR)
  list(APPEND _configure_test_args "-DSDL2_DIR=${SDL2_DIR}")
endif()
# Retired build selectors must stop configuration with migration guidance.
add_test(NAME version_config_rejects
  COMMAND "${CMAKE_COMMAND}" "-DSRC=${PROJECT_SOURCE_DIR}"
          "-DWORK=${CMAKE_BINARY_DIR}/vcr" "-DTEST_ARGS=${_configure_test_args}" -P
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

add_executable(render_horplus render_horplus.c)
target_link_libraries(render_horplus PRIVATE buildengine duke3d::SDL2main)

set(DUKE3D_GRP "" CACHE FILEPATH "DUKE3D.GRP for tests that need game data")
include("${CMAKE_CURRENT_SOURCE_DIR}/render_references.cmake")
duke_render_configure("${CMAKE_CURRENT_BINARY_DIR}/render_reference_settings.cmake")
set(DUKE3D_RENDER_GAMEVERSION "" CACHE STRING "Explicit test profile for modified rendering GRPs")
set(_render_profile_args "")
if(DUKE3D_RENDER_GAMEVERSION)
  set(_render_profile_args -test-gameversion "${DUKE3D_RENDER_GAMEVERSION}")
endif()
# Polymost uses floating-point arithmetic; assertions remain opt-in.
set(DUKE3D_E1L1_POLYMOST_CHECKSUM "" CACHE STRING "Optional platform-specific renderer 2 checksum")

function(add_release_render_tests suffix grp generic)
  foreach(_kind normal tilt classic)
    if(_kind STREQUAL "classic")
      set(_sizes 320x200 640x400 640x480 1280x1024 480x640 1280x720 1600x900)
      set(_exe render_classic2d)
    else()
      set(_sizes view)
      set(_exe render_e1l1)
    endif()
    foreach(_size IN LISTS _sizes)
      if(_kind STREQUAL "normal")
        set(_name render_e1l1${suffix})
      elseif(_kind STREQUAL "tilt")
        set(_name render_e1l1_tilt${suffix})
      else()
        set(_name render_classic2d${suffix}_${_size})
      endif()
      add_test(NAME ${_name} COMMAND "${CMAKE_COMMAND}"
        "-DIDENTIFY=$<TARGET_FILE:game_profile>" "-DEXE=$<TARGET_FILE:${_exe}>"
        "-DGRP=${grp}" "-DKIND=${_kind}" "-DSIZE=${_size}"
        "-DOUT=${CMAKE_CURRENT_BINARY_DIR}/${_name}.bmp" "-DGENERIC=${generic}"
        "-DSETTINGS=${CMAKE_CURRENT_BINARY_DIR}/render_reference_settings.cmake"
        "-DGAMEVERSION=${DUKE3D_RENDER_GAMEVERSION}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/render_reference_run.cmake")
      set_tests_properties(${_name} PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1" TIMEOUT 30
        SKIP_REGULAR_EXPRESSION "No rendering profile:")
    endforeach()
  endforeach()
endfunction()
if(DUKE3D_GRP AND EXISTS "${DUKE3D_GRP}")
  foreach(_renderer 0 2)
    if(_renderer EQUAL 0 OR DUKE3D_POLYMOST)
      add_test(NAME render_horplus_${_renderer}
        COMMAND render_horplus ${_render_profile_args} "${DUKE3D_GRP}" ${_renderer})
      set_tests_properties(render_horplus_${_renderer} PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
        TIMEOUT 30)
    endif()
  endforeach()
  add_release_render_tests("" "${DUKE3D_GRP}" TRUE)

  # Views narrower than a Polymost one before them (which leaves every
  # column's bounds open) draw nothing right of their edge.
  add_test(NAME render_e1l1_narrow
    COMMAND render_e1l1 ${_render_profile_args} "${DUKE3D_GRP}"
            "${CMAKE_CURRENT_BINARY_DIR}/render_e1l1_narrow.bmp" "" 0 narrow)
  set_tests_properties(render_e1l1_narrow PROPERTIES
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
    TIMEOUT 30)

  # The same view through Polymost: renderer 2 (software) and 1 (solid
  # colour). Polymost is floating point, so the checksum may differ by
  # platform; renderer 1 only prints its checksum.
  if(DUKE3D_POLYMOST)
    add_test(NAME render_e1l1_polymost
      COMMAND render_e1l1 ${_render_profile_args} "${DUKE3D_GRP}"
              "${CMAKE_CURRENT_BINARY_DIR}/render_e1l1_polymost.bmp"
              "${DUKE3D_E1L1_POLYMOST_CHECKSUM}" 2)
    add_test(NAME render_e1l1_polymost_solid
      COMMAND render_e1l1 ${_render_profile_args} "${DUKE3D_GRP}"
              "${CMAKE_CURRENT_BINARY_DIR}/render_e1l1_polymost_solid.bmp"
              "" 1)
    # Renderer 2 drawing a render-to-tile view with a power-of-two height.
    add_test(NAME render_e1l1_polymost_tile
      COMMAND render_e1l1 ${_render_profile_args} "${DUKE3D_GRP}"
              "${CMAKE_CURRENT_BINARY_DIR}/render_e1l1_polymost_tile.bmp"
              "" 2 tile)
    # The same tilt and low detail in renderer 2, against renderer 0's.
    add_test(NAME render_e1l1_polymost_tilt
      COMMAND render_e1l1 ${_render_profile_args} "${DUKE3D_GRP}"
              "${CMAKE_CURRENT_BINARY_DIR}/render_e1l1_polymost_tilt.bmp"
              "" 2 tilt)
    # A sprite behind a masked wall (JFBuild 61483f0). Renderer 1 first, so
    # renderer 0's xb1/xb2 aren't left to stand in for dxb1/dxb2.
    add_test(NAME render_maskorder
      COMMAND render_maskorder ${_render_profile_args} "${DUKE3D_GRP}" 1 2 0)
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
    foreach(_path immediate vbo shader)
      add_test(NAME render_horplus_gl_${_path}
        COMMAND render_horplus ${_render_profile_args} "${DUKE3D_GRP}" 3 ${_path})
      set_tests_properties(render_horplus_gl_${_path} PROPERTIES
        ENVIRONMENT "BUILD_WINDOWED=1"
        SKIP_RETURN_CODE 77 TIMEOUT 30)
    endforeach()
    # The game is linked in for its renderer 3 palette code; game.c's main()
    # is renamed (below), as in con_compile.
    add_executable(render_gl render_gl.c render_gl_game.c
                   "${PROJECT_SOURCE_DIR}/source/game.c")
    target_link_libraries(render_gl PRIVATE duke3d_game duke3d::SDL2main)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/render_gl")
    add_test(NAME render_gl
      COMMAND render_gl ${_render_profile_args} "${DUKE3D_GRP}" "${CMAKE_CURRENT_BINARY_DIR}/render_gl")
    set_tests_properties(render_gl PROPERTIES
      ENVIRONMENT "BUILD_WINDOWED=1"
      SKIP_RETURN_CODE 77
      TIMEOUT 30)
    # The VIDEO page's live renderer switch, from a run with no GL context
    # yet, its modes, and the fade around them.
    add_test(NAME render_gl_video
      COMMAND render_gl ${_render_profile_args} "${DUKE3D_GRP}" "${CMAKE_CURRENT_BINARY_DIR}/render_gl" video)
    set_tests_properties(render_gl_video PROPERTIES
      ENVIRONMENT "BUILD_WINDOWED=1"
      SKIP_RETURN_CODE 77
      TIMEOUT 60)
    # Supersampling on the shader path: the frame shown is the large one
    # box-filtered down, its sizes, clamp and the settings that turn it off.
    add_test(NAME render_gl_supersample
      COMMAND render_gl ${_render_profile_args} "${DUKE3D_GRP}" "${CMAKE_CURRENT_BINARY_DIR}/render_gl" supersample)
    set_tests_properties(render_gl_supersample PROPERTIES
      ENVIRONMENT "BUILD_WINDOWED=1"
      SKIP_RETURN_CODE 77
      TIMEOUT 60)
    # Texture filtering on every draw path: close to nearest, translucency
    # and fades kept, nearest again byte for byte, with supersampling too.
    add_test(NAME render_gl_filter
      COMMAND render_gl ${_render_profile_args} "${DUKE3D_GRP}" "${CMAKE_CURRENT_BINARY_DIR}/render_gl" filter)
    set_tests_properties(render_gl_filter PROPERTIES
      ENVIRONMENT "BUILD_WINDOWED=1"
      SKIP_RETURN_CODE 77
      TIMEOUT 90)
  endif()

  # Boots the real game to its title loop and into E1L1, headless. It uses
  # matching CON files extracted from the base archive.
  if(Python3_Interpreter_FOUND)
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
              "-DPYTHON=${Python3_EXECUTABLE}"
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

foreach(_profile 1.4 1.5)
  set(_con_dir "${CMAKE_CURRENT_BINARY_DIR}/con_compile_${_profile}")
  foreach(_con game.con DEFS.CON USER.CON)
    string(TOLOWER "${_con}" _src)
    configure_file("${PROJECT_SOURCE_DIR}/testdata/${_src}" "${_con_dir}/${_con}" COPYONLY)
  endforeach()
  file(READ "${PROJECT_SOURCE_DIR}/testdata/game.con" _game_con)
  file(WRITE "${_con_dir}/prefix.con" "action CONCOMPILE_PREFIX 0 1 1 1 1\r\n${_game_con}")
  add_test(NAME con_compile_${_profile} COMMAND con_compile -test-gameversion ${_profile})
  set_tests_properties(con_compile_${_profile} PROPERTIES WORKING_DIRECTORY "${_con_dir}" TIMEOUT 30)
endforeach()

# Game helper functions against the original DOS libraries. Needs no data.
add_executable(game_helpers game_helpers.c "${_game_c}")
target_link_libraries(game_helpers PRIVATE duke3d_game)
add_test(NAME game_helpers COMMAND game_helpers)

add_executable(gamepad gamepad.c "${_game_c}")
target_link_libraries(gamepad PRIVATE duke3d_game duke3d::SDL2main)
add_test(NAME gamepad COMMAND gamepad)
set_tests_properties(gamepad PROPERTIES
  ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS=1"
  SKIP_RETURN_CODE 77 TIMEOUT 30)

if(UNIX)
  add_executable(input_defaults input_defaults.c "${_game_c}")
  target_link_libraries(input_defaults PRIVATE duke3d_game)
  add_test(NAME input_defaults COMMAND input_defaults)
  set_tests_properties(input_defaults PROPERTIES
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;BUILD_WINDOWED=1"
    TIMEOUT 30)
endif()

# The small full-screen HUD (screen_size 4) at several 8-bit modes: in
# widescreen modes health sits at the screen's left edge and ammo and the
# inventory at its right; 4:3 and tall modes keep their layout. Then a
# deathmatch's frag bar and full status bar, stretched to the full width.
# Each frame is drawn between guard bytes, so a write
# past either end of the 8-bit frame fails. Pinned checksums, hex, of the
# frame without and with an inventory item and of the bars; empty just
# prints them. Needs game data.
add_executable(minihud minihud.c "${_game_c}")
target_link_libraries(minihud PRIVATE duke3d_game)
if(DUKE3D_GRP AND EXISTS "${DUKE3D_GRP}")
  foreach(_pin "320x200;f2070c6d;f191ac15;12e20f57" "640x480;3444f9f9;fab217a1;7e4e5ac7"
               "1280x1024;b324dd41;8b7c4151;b110fc47" "480x640;6b424f6e;b6211dae;46b43892"
               "1280x720;d08e6033;f49c6dcd;656a643e" "1600x900;742b8451;ac001fed;f6bfa15c"
               "1600x675;43b810d6;14f9723b;86d25d4a")
    list(GET _pin 0 _size)
    list(GET _pin 1 _plain)
    list(GET _pin 2 _inv)
    list(GET _pin 3 _bars)
    string(REPLACE "x" ";" _wh "${_size}")
    add_test(NAME minihud_${_size}
      COMMAND "${CMAKE_COMMAND}" "-DIDENTIFY=$<TARGET_FILE:game_profile>"
        "-DEXE=$<TARGET_FILE:minihud>" "-DGRP=${DUKE3D_GRP}"
        "-DOUT=${CMAKE_CURRENT_BINARY_DIR}/minihud_${_size}" -DKIND=minihud "-DSIZE=${_size}"
        "-DMINIREF1=${_plain}" "-DMINIREF2=${_inv}" "-DMINIREF3=${_bars}"
        "-DGAMEVERSION=${DUKE3D_RENDER_GAMEVERSION}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/render_reference_run.cmake")
    set_tests_properties(minihud_${_size} PROPERTIES
      SKIP_REGULAR_EXPRESSION "No rendering profile:"
      ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
      TIMEOUT 30)
  endforeach()
endif()

add_executable(net_game net_game.c "${_game_c}")
target_link_libraries(net_game PRIVATE duke3d_game)

  add_executable(player_death player_death.c "${_game_c}")
  target_link_libraries(player_death PRIVATE duke3d_game)
  add_test(NAME player_death COMMAND player_death)
  set_tests_properties(player_death PROPERTIES
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 30)


    # Private game.c copy: ending automation is absent from the shipped game.
    configure_file("${_game_c}" "${CMAKE_CURRENT_BINARY_DIR}/episode_game.c" COPYONLY)
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/episode_game.c" PROPERTIES
      COMPILE_DEFINITIONS "SDL_MAIN_HANDLED;main=duke3d_main;DUKE3D_EPISODE_TEST=1")
    add_executable(v099_episode_end v099_episode_end.c "${CMAKE_CURRENT_BINARY_DIR}/episode_game.c")
    target_link_libraries(v099_episode_end PRIVATE duke3d_game)

# The private copy reports the first game tic; the shipped executable has no hook.
configure_file("${_game_c}" "${CMAKE_CURRENT_BINARY_DIR}/boot_game_impl.c" COPYONLY)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/boot_game_impl.c" PROPERTIES
  COMPILE_DEFINITIONS "SDL_MAIN_HANDLED;main=duke3d_main;DUKE3D_BOOT_TEST=1")
add_executable(boot_game boot_game.c "${CMAKE_CURRENT_BINARY_DIR}/boot_game_impl.c")
target_link_libraries(boot_game PRIVATE duke3d_game)

if(DUKE3D_ROUTEBOT)
  add_executable(routebot_test routebot.c "${_game_c}")
  target_link_libraries(routebot_test PRIVATE duke3d_game)
  add_test(NAME routebot COMMAND routebot_test)
endif()

add_executable(demo_writer demo_writer.c "${_game_c}")
target_link_libraries(demo_writer PRIVATE duke3d_game)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/demo_writer_run")
add_test(NAME demo_writer COMMAND demo_writer)
set_tests_properties(demo_writer PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/demo_writer_run"
  PASS_REGULAR_EXPRESSION "demo writer checks reached completion" FAIL_REGULAR_EXPRESSION "FAIL;ERROR")


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
if(Python3_Interpreter_FOUND)
  add_test(NAME capture_tools COMMAND "${Python3_EXECUTABLE}" -I
    "${CMAKE_CURRENT_SOURCE_DIR}/capture_tools.py")
endif()

function(add_save_hardening_test name data save version)
  if(EXISTS "${save}")
    if(ARGC GREATER 4)
      set(_reader --codec "$<TARGET_FILE:savegame_codec>")
    else()
      set(_reader --duke3d "$<TARGET_FILE:duke3d>" --data "${data}")
    endif()
    add_test(NAME save_hardening_${name}
      COMMAND "${Python3_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}/savegame_hardening.py"
        ${_reader} --save "${save}"
        --version "${version}" --work "${CMAKE_BINARY_DIR}/sh_${name}")
    set_tests_properties(save_hardening_${name} PROPERTIES TIMEOUT 180)
  endif()
endfunction()
if(DUKE3D_DOS_SAVES_DIR AND Python3_Interpreter_FOUND)
  # 1.4 and 1.5 share the DOS block layout. This codec check deliberately
  # bypasses the game's edition header check and needs no Atomic GRP.
  file(GLOB_RECURSE _codec_saves "${DUKE3D_DOS_SAVES_DIR}/*")
  list(FILTER _codec_saves INCLUDE REGEX "\\.[sS][aA][vV]$")
  list(SORT _codec_saves)
  if(_codec_saves)
    list(GET _codec_saves 0 _codec_save)
    add_save_hardening_test(codec "" "${_codec_save}" 1.5 codec)
  endif()
endif()
if(DUKE3D_ATOMIC_GRP AND EXISTS "${DUKE3D_ATOMIC_GRP}" AND
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
    get_filename_component(_atomic_data "${DUKE3D_ATOMIC_GRP}" DIRECTORY)
    add_save_hardening_test(v15 "${_atomic_data}" "${_first}" 1.5)
    add_dos_save_test(dos_save_refuse refuse "${_first}")
    add_dos_save_test(dos_save_hostile hostile "${_first}")
  endif()
endif()

# 1.3D builds: compile the original 1.3D CONs and compare the bytecode with a
# DOS save made by the original executable. Needs the 1.3D data, so the tests
# are only registered when its folder is set (docs/v13d/reference.md).
set(DUKE3D_V13D_DATA_DIR "" CACHE PATH "Duke Nukem 3D 1.3D full release folder")
set(DUKE3D_V13D_SW_DATA_DIR "" CACHE PATH "Duke Nukem 3D 1.3D shareware folder")
if(Python3_Interpreter_FOUND)
  foreach(_v13d_edition full sw)
    if(_v13d_edition STREQUAL "full")
      set(_v13d_data "${DUKE3D_V13D_DATA_DIR}")
      set(_v13d_profile 1.3d)
      set(_v13d_byte 103)
    else()
      set(_v13d_data "${DUKE3D_V13D_SW_DATA_DIR}")
      set(_v13d_profile 1.3d-shareware)
      set(_v13d_byte 14)
    endif()
  if(_v13d_data)
    add_test(NAME con_compile_v13d_${_v13d_edition}
      COMMAND "${CMAKE_COMMAND}"
              "-DDATA=${_v13d_data}" "-DEDITION=${_v13d_edition}"
              "-DCON_COMPILE=$<TARGET_FILE:con_compile>"
              "-DPYTHON=${Python3_EXECUTABLE}"
              "-DDECODER=${CMAKE_CURRENT_SOURCE_DIR}/v13d_save_decode.py"
              "-DWORK=${CMAKE_BINARY_DIR}/cc13_${_v13d_edition}"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/con_compile_v13d.cmake")
    set_tests_properties(con_compile_v13d_${_v13d_edition} PROPERTIES TIMEOUT 120)
  endif()
  if(_v13d_data)
    add_test(NAME game_boot_v13d_${_v13d_edition}
      COMMAND "${CMAKE_COMMAND}"
              "-DDUKE3D=$<TARGET_FILE:boot_game>"
              "-DDATA=${_v13d_data}" "-DEDITION=${_v13d_edition}"
              "-DWORK=${CMAKE_BINARY_DIR}/gb13_${_v13d_edition}"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/game_boot_v13d.cmake")
    set_tests_properties(game_boot_v13d_${_v13d_edition} PROPERTIES TIMEOUT 240)
    add_save_hardening_test(v13d_${_v13d_edition} "${_v13d_data}" "${_v13d_data}/refsaves/e1l1start.SAV" 1.3D)
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
                "-DWORK=${CMAKE_BINARY_DIR}/ds13_${_demo}_${_v13d_edition}"
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
              "-DWORK=${CMAKE_BINARY_DIR}/dsw13_${_v13d_edition}"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/demo_sync_v13d.cmake")
    set_tests_properties(demo_switch_v13d_${_v13d_edition} PROPERTIES TIMEOUT 400)
    add_test(NAME demo_refuse_v13d_${_v13d_edition}
      COMMAND "${CMAKE_COMMAND}"
              "-DDUKE3D=$<TARGET_FILE:duke3d>"
              "-DDATA=${_v13d_data}" "-DEDITION=${_v13d_edition}"
              -DDEMO=1 -DMODE=refuse
              "-DWORK=${CMAKE_BINARY_DIR}/dr13_${_v13d_edition}"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/demo_sync_v13d.cmake")
    set_tests_properties(demo_refuse_v13d_${_v13d_edition} PROPERTIES TIMEOUT 120)
    if(_v13d_edition STREQUAL "sw")
      add_test(NAME demo_fullonly_v13d_sw
        COMMAND "${CMAKE_COMMAND}"
                "-DDUKE3D=$<TARGET_FILE:duke3d>"
                "-DDATA=${_v13d_data}" "-DEDITION=${_v13d_edition}"
                -DDEMO=1 -DMODE=fullonly
                "-DWORK=${CMAKE_BINARY_DIR}/df13_${_v13d_edition}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/demo_sync_v13d.cmake")
      set_tests_properties(demo_fullonly_v13d_sw PROPERTIES TIMEOUT 120)
    endif()
  endif()
  if(_v13d_data AND EXISTS "${_v13d_data}/refsaves/e1l1start.SAV")
    # Its own directory: lzw_roundtrip writes a scratch file in its working
    # directory, and the plain lzw_roundtrip test may run at the same time.
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lzw_dos_${_v13d_edition}")
    add_test(NAME lzw_dos_v13d_${_v13d_edition}
      COMMAND lzw_roundtrip -test-gameversion ${_v13d_profile} "${_v13d_data}/refsaves/e1l1start.SAV"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lzw_dos_${_v13d_edition}")
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
                  "-DNAME=${_name}" "-DWORK=${CMAKE_BINARY_DIR}/sv13_${_name}_${_v13d_edition}"
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
                "-DOTHER=${_v13d_other}" "-DWORK=${CMAKE_BINARY_DIR}/sr13_${_v13d_edition}"
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
              "-DA=$<TARGET_FILE:net_game>" "-DA_DATA=${_v13d_data}"
              "-DB=$<TARGET_FILE:net_game>" "-DB_DATA=${_v13d_data}"
              -DPORT=${_net_port} -DMODE=connect "-DARGS=/v1 /l1"
              "-DWORK=${CMAKE_BINARY_DIR}/nc13_${_v13d_edition}"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/net_v13d.cmake")
    set_tests_properties(net_connect_v13d_${_v13d_edition} PROPERTIES TIMEOUT 90)
    # One peer changes renderer and resolution as the VIDEO page does, every
    # 20 frames (OpenGL fails headless; BUILD_WINDOWED keeps both windowed):
    # still in sync.
    math(EXPR _net_cycle_port "${_net_port} + 50")
    add_test(NAME net_videocycle_v13d_${_v13d_edition}
      COMMAND "${CMAKE_COMMAND}"
              "-DA=$<TARGET_FILE:net_game>" "-DA_DATA=${_v13d_data}"
              "-DB=$<TARGET_FILE:net_game>" "-DB_DATA=${_v13d_data}"
              -DPORT=${_net_cycle_port} -DMODE=connect "-DARGS=/v1 /l1"
              "-DA_ARGS=-videocycle 20" "-DA_EXPECT=VIDEOCYCLE renderer [0-9] -> [12].*drawn at 1600x900"
              "-DWORK=${CMAKE_BINARY_DIR}/nv13_${_v13d_edition}"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/net_v13d.cmake")
    set_tests_properties(net_videocycle_v13d_${_v13d_edition} PROPERTIES TIMEOUT 90)
    math(EXPR _protocol_port "${_net_port} + 500")
    add_protocol_refusal_tests(v13d_${_v13d_edition} "${_v13d_data}" ${_protocol_port} ${_v13d_byte})
    set(_net_n 0)
    foreach(_peer IN LISTS DUKE3D_NET_PEERS)
      string(REGEX REPLACE "=.*" "" _peer_exe "${_peer}")
      string(REGEX REPLACE "^[^=]*=" "" _peer_data "${_peer}")
      math(EXPR _net_n "${_net_n} + 1")
      math(EXPR _net_port "${_net_port} + 2")
      add_test(NAME net_refuse_v13d_${_v13d_edition}_${_net_n}
        COMMAND "${CMAKE_COMMAND}"
                "-DA=$<TARGET_FILE:net_game>" "-DA_DATA=${_v13d_data}"
                "-DB=${_peer_exe}" "-DB_DATA=${_peer_data}"
                -DPORT=${_net_port} -DMODE=refuse
                "-DWORK=${CMAKE_BINARY_DIR}/nr13_${_net_n}_${_v13d_edition}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/net_v13d.cmake")
      set_tests_properties(net_refuse_v13d_${_v13d_edition}_${_net_n} PROPERTIES TIMEOUT 90)
    endforeach()
  endif()
  endforeach()
  # The edition check must name a wrong edition, not fail some other way.
  if(DUKE3D_V13D_DATA_DIR)
    add_test(NAME v13d_edition_mismatch
      COMMAND "${CMAKE_COMMAND}" "-DDIR=${DUKE3D_V13D_DATA_DIR}" -DEDITION=sw
              -P "${CMAKE_CURRENT_SOURCE_DIR}/v13d_check.cmake")
    set_tests_properties(v13d_edition_mismatch PROPERTIES
      PASS_REGULAR_EXPRESSION "wrong edition: expected sw")
  endif()
endif()

# 1.1, 1.0 and 0.99 shareware builds: the same scripts as the 1.3D tests, with the
# editions sw11, sw10 and sw099 (docs/v10-v11/ and docs/v099/).
set(DUKE3D_V11_SW_DATA_DIR "" CACHE PATH "Duke Nukem 3D 1.1 shareware folder")
set(DUKE3D_V10_SW_DATA_DIR "" CACHE PATH "Duke Nukem 3D 1.0 shareware folder")
set(DUKE3D_V099_SW_DATA_DIR "" CACHE PATH "Duke Nukem 3D 0.99 beta shareware folder")
if(Python3_Interpreter_FOUND)
  foreach(_vold_release 0.99 1.0 1.1)
  if(_vold_release STREQUAL "1.1")
    set(_vold_byte 12)
    set(_vold_edition sw11)
    set(_vold_name v11_sw)
    set(_vold_data "${DUKE3D_V11_SW_DATA_DIR}")
  elseif(_vold_release STREQUAL "1.0")
    set(_vold_byte 10)
    set(_vold_edition sw10)
    set(_vold_name v10_sw)
    set(_vold_data "${DUKE3D_V10_SW_DATA_DIR}")
  else()
    set(_vold_byte 9)
    set(_vold_edition sw099)
    set(_vold_name v099_sw)
    set(_vold_data "${DUKE3D_V099_SW_DATA_DIR}")
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
  endif()
  if(_vold_data)
    add_test(NAME game_boot_${_vold_name}
      COMMAND "${CMAKE_COMMAND}" "-DDUKE3D=$<TARGET_FILE:boot_game>"
              "-DDATA=${_vold_data}" "-DEDITION=${_vold_edition}"
              "-DWORK=${CMAKE_BINARY_DIR}/gb_${_vold_name}"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/game_boot_v13d.cmake")
    set_tests_properties(game_boot_${_vold_name} PROPERTIES TIMEOUT 240)
    add_save_hardening_test(${_vold_name} "${_vold_data}" "${_vold_data}/refsaves/e1l1start.SAV" "${_vold_release}")
  endif()
  if(_vold_data AND _vold_edition STREQUAL "sw099")
    if(EXISTS "${_vold_data}/refdemos/E1L5.DMO")
      add_test(NAME episode_end_v099_sw
        COMMAND "${CMAKE_COMMAND}" "-DDUKE3D=$<TARGET_FILE:v099_episode_end>"
          "-DDATA=${_vold_data}" "-DDEMO=${_vold_data}/refdemos/E1L5.DMO"
          "-DWORK=${CMAKE_BINARY_DIR}/ee_v099" -P "${CMAKE_CURRENT_SOURCE_DIR}/episode_end_v099.cmake")
      set_tests_properties(episode_end_v099_sw PROPERTIES TIMEOUT 140)
    endif()
    add_test(NAME v099_render_camera
      COMMAND v099_render_camera "${_vold_data}/DUKE3D.GRP")
    set_tests_properties(v099_render_camera PROPERTIES
      ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 30)
    include("${CMAKE_CURRENT_SOURCE_DIR}/data/v13d_demo_refs.cmake")
    foreach(_level RANGE 1 6)
      if(EXISTS "${_vold_data}/refdemos/E1L${_level}.DMO")
        add_test(NAME demo_sync_v099_sw_e1l${_level}
          COMMAND "${CMAKE_COMMAND}" "-DDUKE3D=$<TARGET_FILE:duke3d>"
            "-DDATA=${_vold_data}" -DEDITION=sw099 -DDEMO=1 "-DREFNAME=E1L${_level}"
            "-DDEMOFILE=${_vold_data}/refdemos/E1L${_level}.DMO"
            "-DDEMOGOD=${V13D_DEMO_SW099_E1L${_level}_DEMOGOD}"
            "-DREFS=${CMAKE_CURRENT_SOURCE_DIR}/data/v13d_demo_refs.cmake"
            "-DWORK=${CMAKE_BINARY_DIR}/ds_v099_e1l${_level}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/demo_sync_v13d.cmake")
        set_tests_properties(demo_sync_v099_sw_e1l${_level} PROPERTIES TIMEOUT 400)
      endif()
    endforeach()
    if(DUKE3D_ROUTEBOT)
      add_test(NAME routebot_record_v099_sw
        COMMAND "${CMAKE_COMMAND}" "-DDUKE3D=$<TARGET_FILE:duke3d>"
          "-DDATA=${_vold_data}" "-DWORK=${CMAKE_BINARY_DIR}/routebot_record"
          -P "${CMAKE_CURRENT_SOURCE_DIR}/routebot_record.cmake")
      set_tests_properties(routebot_record_v099_sw PROPERTIES TIMEOUT 90)
      add_test(NAME routebot_record_god_v099_sw
        COMMAND "${CMAKE_COMMAND}" "-DDUKE3D=$<TARGET_FILE:duke3d>"
          "-DDATA=${_vold_data}" -DDEMOGOD=ON "-DWORK=${CMAKE_BINARY_DIR}/routebot_record_god"
          -P "${CMAKE_CURRENT_SOURCE_DIR}/routebot_record.cmake")
      set_tests_properties(routebot_record_god_v099_sw PROPERTIES TIMEOUT 90)
    endif()
    foreach(_mode refuse roundtrip)
      add_test(NAME demo_${_mode}_${_vold_name}
        COMMAND "${CMAKE_COMMAND}" "-DDUKE3D=$<TARGET_FILE:duke3d>"
          "-DDATA=${_vold_data}" -DEDITION=sw099 -DDEMO=1 "-DMODE=${_mode}"
          "-DPYTHON=${Python3_EXECUTABLE}"
          "-DWORK=${CMAKE_BINARY_DIR}/d${_mode}_${_vold_name}"
          -P "${CMAKE_CURRENT_SOURCE_DIR}/demo_sync_v13d.cmake")
      set_tests_properties(demo_${_mode}_${_vold_name} PROPERTIES TIMEOUT 120)
    endforeach()
  endif()
  # The original demos, played to the end and compared with the DOS state.
  if(_vold_data AND NOT _vold_edition STREQUAL "sw099")
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
        string(REGEX REPLACE "^sw(099|1[01])_" "" _name "${_ref}")
        add_test(NAME dos_save_${_vold_name}_${_name}
          COMMAND "${CMAKE_COMMAND}" -DMODE=load "-DNAME=${_ref}"
                  "-DDUKE3D=$<TARGET_FILE:duke3d>"
                  "-DDATA=${_vold_data}" "-DEDITION=${_vold_edition}"
                  "-DREFS=${CMAKE_CURRENT_SOURCE_DIR}/data/v13d_save_refs.cmake"
                  "-DPYTHON=${Python3_EXECUTABLE}"
                  "-DDECODER=${CMAKE_CURRENT_SOURCE_DIR}/v13d_save_decode.py"
                  "-DWORK=${CMAKE_BINARY_DIR}/sv_${_vold_name}_${_name}"
                  -P "${CMAKE_CURRENT_SOURCE_DIR}/dos_save_v13d.cmake")
        set_tests_properties(dos_save_${_vold_name}_${_name} PROPERTIES TIMEOUT 120)
      endif()
    endforeach()
    # The other release's and the 1.3D shareware saves, when their folders are
    # set, and a 1.5 one must be refused (rulings.md: 1.1's BYTEVERSION 12
    # collides with neither; 1.0 has none and goes by its layout).
    if(_vold_edition STREQUAL "sw11" OR _vold_edition STREQUAL "sw099")
      set(_vold_other_dir "${DUKE3D_V10_SW_DATA_DIR}")
    else()
      set(_vold_other_dir "${DUKE3D_V11_SW_DATA_DIR}")
    endif()
    set(_vold_others "")
    foreach(_other "${_vold_other_dir}" "${DUKE3D_V13D_SW_DATA_DIR}" "${DUKE3D_V099_SW_DATA_DIR}")
      if(_other AND NOT _other STREQUAL _vold_data AND EXISTS "${_other}/refsaves/e1l1start.SAV")
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
  if(_vold_data AND NOT _vold_edition STREQUAL "sw099")
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
                "-DA=$<TARGET_FILE:net_game>" "-DA_DATA=${_vold_data}"
                "-DB=$<TARGET_FILE:net_game>" "-DB_DATA=${_vold_data}"
                -DPORT=${_net_port} -DMODE=connect "-DARGS=${_net_args}"
                "-DWORK=${CMAKE_BINARY_DIR}/nc_${_vold_name}_${_mode}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/net_v13d.cmake")
      set_tests_properties(${_net_test} PROPERTIES TIMEOUT 90)
      math(EXPR _net_port "${_net_port} + 2")
    endforeach()
    math(EXPR _protocol_port "${_net_port} + 500")
    add_protocol_refusal_tests(${_vold_name} "${_vold_data}" ${_protocol_port} ${_vold_byte})
    set(_net_n 0)
    foreach(_peer IN LISTS DUKE3D_NET_PEERS)
      string(REGEX REPLACE "=.*" "" _peer_exe "${_peer}")
      string(REGEX REPLACE "^[^=]*=" "" _peer_data "${_peer}")
      math(EXPR _net_n "${_net_n} + 1")
      math(EXPR _net_port "${_net_port} + 2")
      add_test(NAME net_refuse_${_vold_name}_${_net_n}
        COMMAND "${CMAKE_COMMAND}"
                "-DA=$<TARGET_FILE:net_game>" "-DA_DATA=${_vold_data}"
                "-DB=${_peer_exe}" "-DB_DATA=${_peer_data}"
                -DPORT=${_net_port} -DMODE=refuse
                "-DWORK=${CMAKE_BINARY_DIR}/nr_${_vold_name}_${_net_n}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/net_v13d.cmake")
      set_tests_properties(net_refuse_${_vold_name}_${_net_n} PROPERTIES TIMEOUT 90)
    endforeach()
  endif()
  # Check every other folder that is available, even if only one is set.
  set(_vold_other_dirs "")
  foreach(_other "${DUKE3D_V11_SW_DATA_DIR}" "${DUKE3D_V10_SW_DATA_DIR}"
                 "${DUKE3D_V13D_SW_DATA_DIR}" "${DUKE3D_V099_SW_DATA_DIR}")
    if(_other AND NOT _other STREQUAL _vold_data)
      if(_vold_other_dirs)
        string(APPEND _vold_other_dirs "$<SEMICOLON>")
      endif()
      string(APPEND _vold_other_dirs "${_other}")
    endif()
  endforeach()
  if(_vold_other_dirs)
    string(REPLACE "_sw" "" _vold_short "${_vold_name}")
    add_test(NAME ${_vold_short}_edition_mismatch
      COMMAND "${CMAKE_COMMAND}" "-DEDITION=${_vold_edition}"
              "-DOTHERS=${_vold_other_dirs}"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/v13d_check.cmake")
  endif()
  endforeach()
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

add_executable(input_polling input_polling.c "${_game_c}")
target_link_libraries(input_polling PRIVATE duke3d_game duke3d::SDL2main)
add_test(NAME input_polling COMMAND input_polling)
set_tests_properties(input_polling PROPERTIES
  ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;BUILD_WINDOWED=1;BUILD_SDLJOYSTICK=none"
  TIMEOUT 30)

add_executable(input_aim input_aim.c "${PROJECT_SOURCE_DIR}/source/inputaim.c")
target_include_directories(input_aim PRIVATE "${PROJECT_SOURCE_DIR}/source")
if(NOT WIN32)
  target_link_libraries(input_aim PRIVATE m)
endif()
add_test(NAME input_aim COMMAND input_aim)

add_executable(joystick_polling joystick_polling.c "${_game_c}")
target_link_libraries(joystick_polling PRIVATE duke3d_game duke3d::SDL2main)
add_test(NAME joystick_polling COMMAND joystick_polling)
set_tests_properties(joystick_polling PROPERTIES
  ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS=1"
  SKIP_RETURN_CODE 77 TIMEOUT 30)

add_executable(input_render input_render.c "${_game_c}")
target_link_libraries(input_render PRIVATE duke3d_game duke3d::SDL2main)
add_test(NAME input_render COMMAND input_render)
set_tests_properties(input_render PROPERTIES
  ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
  TIMEOUT 30)

add_executable(game_profile game_profile.c)
target_link_libraries(game_profile PRIVATE duke3d_profile)
add_test(NAME game_profile COMMAND game_profile "${CMAKE_CURRENT_BINARY_DIR}/profile-test.grp")
add_executable(game_version_args game_version_args.c)
target_link_libraries(game_version_args PRIVATE duke3d_profile)
add_test(NAME game_version_args COMMAND game_version_args)

add_executable(game_state game_state.c)
target_link_libraries(game_state PRIVATE duke3d_game_options buildengine audiolib)
add_test(NAME game_state COMMAND game_state)

foreach(_profile 0.99 1.0 1.1 1.3d-shareware 1.3d 1.4 1.5)
  foreach(_case engine_inside engine_trace lzw_roundtrip)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${_case}_${_profile}")
    add_test(NAME ${_case}_${_profile} COMMAND ${_case} -test-gameversion ${_profile})
    set_tests_properties(${_case}_${_profile} PROPERTIES
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${_case}_${_profile}")
  endforeach()
endforeach()

add_executable(game_release game_release.c "${_game_c}")
target_link_libraries(game_release PRIVATE duke3d_game)
foreach(_profile 0.99 1.0 1.1 1.3d-shareware 1.3d 1.4 1.5)
  add_test(NAME game_release_${_profile} COMMAND game_release -test-gameversion ${_profile})
endforeach()

foreach(_case early_scenarios player_prediction v099_actors v099_player v099_sector v099_render_camera con_runtime_v099)
  if(NOT TARGET ${_case})
    add_executable(${_case} ${_case}.c "${_game_c}")
    target_link_libraries(${_case} PRIVATE duke3d_game)
  endif()
endforeach()
foreach(_profile 1.0 1.1)
  foreach(_case early_scenarios player_prediction)
    add_test(NAME ${_case}_${_profile} COMMAND ${_case} -test-gameversion ${_profile})
    set_tests_properties(${_case}_${_profile} PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 30)
  endforeach()
endforeach()
foreach(_case v099_actors v099_player v099_sector con_runtime_v099)
  add_test(NAME ${_case}_runtime COMMAND ${_case} -test-gameversion 0.99)
  set_tests_properties(${_case}_runtime PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 30)
endforeach()

foreach(_profile 0.99 1.0 1.1 1.3d-shareware 1.3d 1.4 1.5)
  add_test(NAME savegame_codec_${_profile} COMMAND savegame_codec -test-gameversion ${_profile})
endforeach()

foreach(_case v099_game v099_menus)
  if(NOT TARGET ${_case})
    add_executable(${_case} ${_case}.c "${_game_c}")
    target_link_libraries(${_case} PRIVATE duke3d_game)
  endif()
  add_test(NAME ${_case}_runtime COMMAND ${_case} -test-gameversion 0.99)
  set_tests_properties(${_case}_runtime PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 30)
endforeach()
foreach(_profile 0.99 1.0 1.1 1.3d-shareware 1.3d 1.4 1.5)
  foreach(_case demo_writer game_helpers gamepad)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${_case}_${_profile}")
    add_test(NAME ${_case}_${_profile} COMMAND ${_case} -test-gameversion ${_profile})
    set_tests_properties(${_case}_${_profile} PROPERTIES
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${_case}_${_profile}"
      ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 30 SKIP_RETURN_CODE 77)
  endforeach()
endforeach()

add_executable(conveyor_runtime conveyor_runtime.c "${_game_c}")
target_link_libraries(conveyor_runtime PRIVATE duke3d_game)
foreach(_profile 0.99 1.0 1.1 1.3d-shareware 1.3d 1.4 1.5)
  add_test(NAME conveyor_runtime_${_profile} COMMAND conveyor_runtime -test-gameversion ${_profile})
endforeach()

set(DUKE3D_V14_DATA_DIR "" CACHE PATH "Original Duke Nukem 3D 1.4 folder (optional)")
if(Python3_Interpreter_FOUND)
  set(_cli_fixtures "")
  foreach(_pair "0.99|${DUKE3D_V099_SW_DATA_DIR}" "1.0|${DUKE3D_V10_SW_DATA_DIR}"
                "1.1|${DUKE3D_V11_SW_DATA_DIR}" "1.3d-shareware|${DUKE3D_V13D_SW_DATA_DIR}"
                "1.3d|${DUKE3D_V13D_DATA_DIR}" "1.4|${DUKE3D_V14_DATA_DIR}")
    string(REPLACE "|" ";" _parts "${_pair}")
    list(GET _parts 0 _profile)
    list(GET _parts 1 _folder)
    if(EXISTS "${_folder}/DUKE3D.GRP")
      list(APPEND _cli_fixtures --fixture "${_profile}=${_folder}/DUKE3D.GRP")
    endif()
  endforeach()
  if(EXISTS "${DUKE3D_ATOMIC_GRP}")
    list(APPEND _cli_fixtures --fixture "1.5=${DUKE3D_ATOMIC_GRP}")
  endif()
  add_test(NAME game_version_cli COMMAND "${CMAKE_COMMAND}"
    "-DDUKE3D=$<TARGET_FILE:duke3d>" "-DPYTHON=${Python3_EXECUTABLE}"
    "-DWORK=${CMAKE_BINARY_DIR}/version_cli" "-DFIXTURES=${_cli_fixtures}" -P "${CMAKE_CURRENT_SOURCE_DIR}/game_version_cli.cmake")
  set_tests_properties(game_version_cli PROPERTIES TIMEOUT 180)
endif()
add_executable(shareware_maps shareware_maps.c "${_game_c}")
target_link_libraries(shareware_maps PRIVATE duke3d_game)
foreach(_profile 0.99 1.0 1.1 1.3d-shareware 1.3d 1.4 1.5)
  add_test(NAME shareware_maps_${_profile} COMMAND "${CMAKE_COMMAND}"
    "-DEXE=$<TARGET_FILE:shareware_maps>" "-DPROFILE=${_profile}"
    "-DWORK=${CMAKE_BINARY_DIR}/maps_${_profile}" -P "${CMAKE_CURRENT_SOURCE_DIR}/shareware_maps.cmake")
endforeach()

function(add_shareware_map_network profile data byteversion port)
  if(EXISTS "${data}/DUKE3D.GRP")
    add_test(NAME net_shareware_map_${profile} COMMAND "${CMAKE_COMMAND}"
      "-DA=$<TARGET_FILE:net_game>" "-DA_DATA=${data}"
      "-DB=$<TARGET_FILE:net_protocol_peer>" "-DB_VERSION=${byteversion}"
      "-DPORT=${port}" -DMODE=maprefuse "-DWORK=${CMAKE_BINARY_DIR}/nm_${profile}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/net_v13d.cmake")
    set_tests_properties(net_shareware_map_${profile} PROPERTIES TIMEOUT 60)
  endif()
endfunction()
add_shareware_map_network(1.0 "${DUKE3D_V10_SW_DATA_DIR}" 10 27200)
add_shareware_map_network(1.1 "${DUKE3D_V11_SW_DATA_DIR}" 12 27208)
add_shareware_map_network(1.3d-shareware "${DUKE3D_V13D_SW_DATA_DIR}" 14 27216)

add_test(NAME runtime_test_matrix COMMAND "${CMAKE_COMMAND}" "-DSRC=${PROJECT_SOURCE_DIR}"
  "-DWORK=${CMAKE_BINARY_DIR}/matrix" "-DTEST_ARGS=${_configure_test_args}"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/runtime_test_matrix.cmake")
set_tests_properties(runtime_test_matrix PROPERTIES TIMEOUT 120)

foreach(_profile 0.99 1.0 1.1 1.3d-shareware 1.3d 1.4 1.5)
  foreach(_case input_defaults input_polling)
    if(TARGET ${_case})
      file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${_case}_${_profile}")
      add_test(NAME ${_case}_${_profile} COMMAND ${_case} -test-gameversion ${_profile})
      set_tests_properties(${_case}_${_profile} PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${_case}_${_profile}"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 30)
    endif()
  endforeach()
endforeach()
foreach(_profile 1.1 1.3d-shareware 1.3d 1.4)
  add_test(NAME player_death_${_profile} COMMAND player_death -test-gameversion ${_profile})
  set_tests_properties(player_death_${_profile} PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 30)
endforeach()

foreach(_retail 1.4 1.5)
  if(_retail STREQUAL "1.4")
    set(_retail_data "${DUKE3D_V14_DATA_DIR}")
    set(_retail_grp "${_retail_data}/DUKE3D.GRP")
    set(_retail_name v14)
    set(_retail_byte 116)
    set(_retail_port 24900)
  else()
    get_filename_component(_retail_data "${DUKE3D_ATOMIC_GRP}" DIRECTORY)
    set(_retail_grp "${DUKE3D_ATOMIC_GRP}")
    set(_retail_name atomic)
    set(_retail_byte 117)
    set(_retail_port 25000)
  endif()
  if(Python3_Interpreter_FOUND AND EXISTS "${_retail_grp}")
    add_test(NAME game_boot_${_retail_name} COMMAND "${CMAKE_COMMAND}"
      "-DDUKE3D=$<TARGET_FILE:boot_game>" "-DDATA=${_retail_data}" "-DGRP=${_retail_grp}"
      "-DPROFILE=${_retail}" "-DWORK=${CMAKE_BINARY_DIR}/gb_${_retail_name}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/game_boot_v13d.cmake")
    set_tests_properties(game_boot_${_retail_name} PROPERTIES TIMEOUT 240)
    add_test(NAME net_connect_${_retail_name} COMMAND "${CMAKE_COMMAND}"
      "-DA=$<TARGET_FILE:net_game>" "-DA_DATA=${_retail_data}"
      "-DB=$<TARGET_FILE:net_game>" "-DB_DATA=${_retail_data}"
      -DPORT=${_retail_port} -DMODE=connect "-DARGS=/v1 /l1"
      "-DWORK=${CMAKE_BINARY_DIR}/nc_${_retail_name}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/net_v13d.cmake")
    set_tests_properties(net_connect_${_retail_name} PROPERTIES TIMEOUT 90)
    math(EXPR _retail_protocol_port "${_retail_port} + 500")
    add_protocol_refusal_tests(${_retail_name} "${_retail_data}" ${_retail_protocol_port} ${_retail_byte})
  endif()
endforeach()

# Two instances of this executable must refuse each other's detected release.
get_filename_component(_atomic_data "${DUKE3D_ATOMIC_GRP}" DIRECTORY)
function(add_cross_release_test name a_data b_data port)
  if(EXISTS "${a_data}/DUKE3D.GRP" AND EXISTS "${b_data}/DUKE3D.GRP")
    add_test(NAME net_cross_${name} COMMAND "${CMAKE_COMMAND}"
      "-DA=$<TARGET_FILE:net_game>" "-DA_DATA=${a_data}"
      "-DB=$<TARGET_FILE:net_game>" "-DB_DATA=${b_data}"
      "-DPORT=${port}" -DMODE=refuse "-DWORK=${CMAKE_BINARY_DIR}/nx_${name}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/net_v13d.cmake")
    set_tests_properties(net_cross_${name} PROPERTIES TIMEOUT 60 RUN_SERIAL TRUE)
  endif()
endfunction()
add_cross_release_test(v10_v11 "${DUKE3D_V10_SW_DATA_DIR}" "${DUKE3D_V11_SW_DATA_DIR}" 28200)
add_cross_release_test(13d_sw_full "${DUKE3D_V13D_SW_DATA_DIR}" "${DUKE3D_V13D_DATA_DIR}" 28208)
add_cross_release_test(13d_atomic "${DUKE3D_V13D_DATA_DIR}" "${_atomic_data}" 28216)

add_test(NAME render_reference_config COMMAND "${CMAKE_COMMAND}"
  "-DSRC=${PROJECT_SOURCE_DIR}" "-DWORK=${CMAKE_BINARY_DIR}/rrc"
  "-DIDENTIFY=$<TARGET_FILE:game_profile>" "-DRENDER=$<TARGET_FILE:render_e1l1>" "-DMINIHUD=$<TARGET_FILE:minihud>"
  "-DGRP_13d=${DUKE3D_V13D_DATA_DIR}/DUKE3D.GRP" "-DGRP_atomic=${DUKE3D_ATOMIC_GRP}"
  "-DTEST_ARGS=${_configure_test_args}" -P "${CMAKE_CURRENT_SOURCE_DIR}/render_reference_config.cmake")
set_tests_properties(render_reference_config PROPERTIES TIMEOUT 120
  ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1")

if(EXISTS "${DUKE3D_V13D_DATA_DIR}/DUKE3D.GRP")
  add_release_render_tests(_13d "${DUKE3D_V13D_DATA_DIR}/DUKE3D.GRP" FALSE)
endif()
if(EXISTS "${DUKE3D_ATOMIC_GRP}")
  add_release_render_tests(_atomic "${DUKE3D_ATOMIC_GRP}" FALSE)
endif()

# Data without a selectable profile needs an explicit test-only override.
get_property(_render_tests DIRECTORY PROPERTY TESTS)
foreach(_test IN LISTS _render_tests)
  if(_test MATCHES "^(render_|minihud_)")
    set_tests_properties(${_test} PROPERTIES SKIP_RETURN_CODE 77)
  endif()
endforeach()

add_executable(con_script_limits con_script_limits.c "${_game_c}")
target_link_libraries(con_script_limits PRIVATE duke3d_game)
add_test(NAME con_script_limits COMMAND "${CMAKE_COMMAND}"
  "-DEXE=$<TARGET_FILE:con_script_limits>" "-DWORK=${CMAKE_CURRENT_BINARY_DIR}"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/con_script_limits.cmake")

if(Python3_Interpreter_FOUND)
  add_test(NAME grp_extent_limits COMMAND "${Python3_EXECUTABLE}" -I
    "${CMAKE_CURRENT_SOURCE_DIR}/grp_extent_limits.py" "$<TARGET_FILE:game_profile>"
    "$<TARGET_FILE:duke3d>" "${CMAKE_BINARY_DIR}/extent_limits")
  set_tests_properties(grp_extent_limits PROPERTIES TIMEOUT 120)
endif()
