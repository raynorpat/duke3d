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
  duke3d_game_options buildengine audiolib duke3d::SDL2_mixer duke3d::SDL2main)
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
set(DUKE3D_E1L1_CHECKSUM "8e9d8814" CACHE STRING
    "Expected render_e1l1 frame checksum (hex); empty just prints it")
if(DUKE3D_GRP AND EXISTS "${DUKE3D_GRP}")
  add_test(NAME render_e1l1
    COMMAND render_e1l1 "${DUKE3D_GRP}"
            "${CMAKE_CURRENT_BINARY_DIR}/render_e1l1.bmp"
            ${DUKE3D_E1L1_CHECKSUM})
  set_tests_properties(render_e1l1 PROPERTIES
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
    TIMEOUT 30)

  # Boots the real game to its title loop and into E1L1, headless.
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

add_test(NAME con_compile COMMAND con_compile WORKING_DIRECTORY "${_con_dir}")
set_tests_properties(con_compile PROPERTIES TIMEOUT 30)

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
    add_dos_save_test(dos_save_refuse refuse "${_first}")
    add_dos_save_test(dos_save_hostile hostile "${_first}")
  endif()
endif()
