# Drives the SDL2 video and timer layer of the Build engine without game
# data. Runs headless via SDL's dummy video driver.
add_executable(sdl_driver_smoke sdl_driver_smoke.c)
target_link_libraries(sdl_driver_smoke PRIVATE buildengine duke3d::SDL2main)

add_test(NAME sdl_driver_smoke COMMAND sdl_driver_smoke)
set_tests_properties(sdl_driver_smoke PROPERTIES
  ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
  TIMEOUT 30)

# Checks DUKE3D_GAMEVER, BUILD_ENGINEREV, BYTEVERSION and the volume macros
# against this configuration's DUKE3D_VERSION. The table is kept here, apart
# from the one that sets them, so a mistake in either shows up.
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

set(DUKE3D_GRP "" CACHE FILEPATH "DUKE3D.GRP for tests that need game data")
set(DUKE3D_E1L1_CHECKSUM "e0517697" CACHE STRING
    "Expected render_e1l1 frame checksum (hex); empty just prints it")
if(DUKE3D_GRP AND EXISTS "${DUKE3D_GRP}")
  add_test(NAME render_e1l1
    COMMAND render_e1l1 "${DUKE3D_GRP}"
            "${CMAKE_CURRENT_BINARY_DIR}/render_e1l1.bmp"
            ${DUKE3D_E1L1_CHECKSUM})
  set_tests_properties(render_e1l1 PROPERTIES
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
    TIMEOUT 30)

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

# The in-repo CONs are 1.5 scripts, which a 1.3D build can't compile.
if(NOT DUKE3D_VERSION STREQUAL "1.3D")
  add_test(NAME con_compile COMMAND con_compile WORKING_DIRECTORY "${_con_dir}")
  set_tests_properties(con_compile PROPERTIES TIMEOUT 30)
endif()

# Game helper functions against the original DOS libraries. Needs no data.
add_executable(game_helpers game_helpers.c "${_game_c}")
target_link_libraries(game_helpers PRIVATE duke3d_game)
add_test(NAME game_helpers COMMAND game_helpers)

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
    add_test(NAME demo_refuse_v13d_${_v13d_edition}
      COMMAND "${CMAKE_COMMAND}"
              "-DDUKE3D=$<TARGET_FILE:duke3d>"
              "-DDATA=${_v13d_data}" "-DEDITION=${_v13d_edition}"
              -DDEMO=1 -DMODE=refuse
              "-DWORK=${CMAKE_BINARY_DIR}/dr13"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/demo_sync_v13d.cmake")
    set_tests_properties(demo_refuse_v13d_${_v13d_edition} PROPERTIES TIMEOUT 120)
  endif()
  if(_v13d_data AND EXISTS "${_v13d_data}/refsaves/e1l1start.SAV")
    # Its own directory: lzw_roundtrip writes a scratch file in its working
    # directory, and the plain lzw_roundtrip test may run at the same time.
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lzw_dos")
    add_test(NAME lzw_dos_v13d_${_v13d_edition}
      COMMAND lzw_roundtrip "${_v13d_data}/refsaves/e1l1start.SAV"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lzw_dos")
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
              -DPORT=${_net_port} -DMODE=connect
              "-DWORK=${CMAKE_BINARY_DIR}/nc13"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/net_v13d.cmake")
    set_tests_properties(net_connect_v13d_${_v13d_edition} PROPERTIES TIMEOUT 90)
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
