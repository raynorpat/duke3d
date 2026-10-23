# Everything but game.c, which holds main(). The con_compile test links
# these objects with its own main().
add_library(duke3d_game OBJECT
  actors.c
  animlib.c
  config.c
  control.c
  gamedef.c
  gamepad.c
  global.c
  keyboard.c
  menues.c
  player.c
  premap.c
  rts.c
  savegame.c
  scriplib.c
  sector.c
  sounds.c)

if(DUKE3D_ROUTEBOT)
  target_sources(duke3d_game PRIVATE routebot.c)
  target_compile_definitions(duke3d_game PUBLIC DUKE3D_ROUTEBOT=1)
endif()

# "build/engine.h" etc. resolve against this directory.
target_include_directories(duke3d_game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(duke3d_game PUBLIC
  duke3d_game_options buildengine audiolib)

# The Windows Saved Games folder (SHGetKnownFolderPath, CoTaskMemFree).
if(WIN32)
  target_sources(duke3d_game PRIVATE win32_savedir.c)
  target_link_libraries(duke3d_game PUBLIC shell32 ole32)
endif()

add_executable(duke3d game.c)
target_link_libraries(duke3d PRIVATE duke3d_game duke3d::SDL2main)

# Animation playback used to crash from an optimizer bug on Mac OS X.
if(APPLE)
  set_source_files_properties(animlib.c PROPERTIES COMPILE_OPTIONS "-O0")
endif()

# The map editor. The target isn't called "build", so it can't clash with
# the build/ binary directory; the executable still is.
add_executable(buildeditor astub.c build/build.c buildcfg.c editormode.c)
set_target_properties(buildeditor PROPERTIES OUTPUT_NAME build)
target_include_directories(buildeditor PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(buildeditor PRIVATE
  duke3d_game_options buildengine duke3d::SDL2main)

# Shared SDL DLLs (vcpkg etc.) must sit next to the executables. Fetched SDL
# is static, and an empty DLL list would make copy_if_different fail.
if(WIN32 AND NOT DUKE3D_SDL2_FETCHED)
  foreach(_exe duke3d buildeditor)
    add_custom_command(TARGET ${_exe} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
              $<TARGET_RUNTIME_DLLS:${_exe}> $<TARGET_FILE_DIR:${_exe}>
      COMMAND_EXPAND_LISTS)
  endforeach()
endif()
