# Finds SDL2, or downloads and builds it statically. Either way, the rest
# of the build links duke3d::SDL2 and duke3d::SDL2main.

include(FetchContent)

set(DUKE3D_SDL2_URL
    "https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz")
set(DUKE3D_SDL2_SHA256
    "5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165")

if(NOT DUKE3D_FETCH_SDL STREQUAL "ON")
  find_package(SDL2 CONFIG QUIET)
endif()

set(DUKE3D_SDL2_FETCHED FALSE)
if(NOT TARGET SDL2::SDL2 AND NOT TARGET SDL2::SDL2-static)
  set(DUKE3D_SDL2_FETCHED TRUE)
endif()

if(DUKE3D_SDL2_FETCHED AND DUKE3D_FETCH_SDL STREQUAL "OFF")
  message(FATAL_ERROR
    "SDL2 not found and DUKE3D_FETCH_SDL=OFF. "
    "Install it (brew install sdl2 / apt install libsdl2-dev) "
    "or set DUKE3D_FETCH_SDL=AUTO.")
endif()

# Vendored libraries are static and don't need their own install rules.
set(BUILD_SHARED_LIBS OFF)

if(DUKE3D_SDL2_FETCHED)
  message(STATUS "duke3d: building SDL2 2.32.10 from source")
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST OFF CACHE BOOL "" FORCE)
  set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL2
    URL "${DUKE3D_SDL2_URL}"
    URL_HASH "SHA256=${DUKE3D_SDL2_SHA256}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  FetchContent_MakeAvailable(SDL2)
else()
  message(STATUS "duke3d: using installed SDL2 ${SDL2_VERSION}")
endif()

add_library(duke3d_sdl2 INTERFACE)
add_library(duke3d::SDL2 ALIAS duke3d_sdl2)
if(TARGET SDL2::SDL2)
  target_link_libraries(duke3d_sdl2 INTERFACE SDL2::SDL2)
else()
  target_link_libraries(duke3d_sdl2 INTERFACE SDL2::SDL2-static)
endif()

add_library(duke3d_sdl2main INTERFACE)
add_library(duke3d::SDL2main ALIAS duke3d_sdl2main)
if(WIN32 AND TARGET SDL2::SDL2main)
  target_link_libraries(duke3d_sdl2main INTERFACE SDL2::SDL2main)
endif()
