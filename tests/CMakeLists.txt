# Drives the SDL2 video and timer layer of the Build engine without game
# data. Runs headless via SDL's dummy video driver.
add_executable(sdl_driver_smoke sdl_driver_smoke.c)
target_link_libraries(sdl_driver_smoke PRIVATE buildengine duke3d::SDL2main)

add_test(NAME sdl_driver_smoke COMMAND sdl_driver_smoke)
set_tests_properties(sdl_driver_smoke PROPERTIES
  ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
  TIMEOUT 30)

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
    set(_cfg_dir ".")
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
