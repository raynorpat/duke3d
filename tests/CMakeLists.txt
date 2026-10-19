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
endif()
