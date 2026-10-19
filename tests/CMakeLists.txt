# Drives the SDL2 video and timer layer of the Build engine without game
# data. Runs headless via SDL's dummy video driver.
add_executable(sdl_driver_smoke sdl_driver_smoke.c)
target_link_libraries(sdl_driver_smoke PRIVATE buildengine duke3d::SDL2main)

# Not registered on Windows: the engine keeps pointers in 32-bit "long"
# there (LLP64), so it builds but crashes at runtime until the 64-bit
# cleanup spec lands. Building it still catches compile/link breakage.
if(NOT WIN32)
  add_test(NAME sdl_driver_smoke COMMAND sdl_driver_smoke)
  set_tests_properties(sdl_driver_smoke PROPERTIES
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;BUILD_WINDOWED=1"
    TIMEOUT 30)
endif()
