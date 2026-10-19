# Finds SDL2 and SDL2_mixer, or downloads and builds them statically.
# Either way, the rest of the build links duke3d::SDL2, duke3d::SDL2main
# and duke3d::SDL2_mixer.

include(FetchContent)

set(DUKE3D_SDL2_URL
    "https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz")
set(DUKE3D_SDL2_SHA256
    "5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165")
set(DUKE3D_SDL2_MIXER_URL
    "https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.1/SDL2_mixer-2.8.1.tar.gz")
set(DUKE3D_SDL2_MIXER_SHA256
    "cb760211b056bfe44f4a1e180cc7cb201137e4d1572f2002cc1be728efd22660")

if(NOT DUKE3D_FETCH_SDL STREQUAL "ON")
  find_package(SDL2 CONFIG QUIET)
  find_package(SDL2_mixer CONFIG QUIET)
endif()

set(DUKE3D_SDL2_FETCHED FALSE)
set(DUKE3D_SDL2_MIXER_FETCHED FALSE)
if(NOT TARGET SDL2::SDL2 AND NOT TARGET SDL2::SDL2-static)
  set(DUKE3D_SDL2_FETCHED TRUE)
endif()
if(NOT TARGET SDL2_mixer::SDL2_mixer AND NOT TARGET SDL2_mixer::SDL2_mixer-static)
  set(DUKE3D_SDL2_MIXER_FETCHED TRUE)
endif()

if((DUKE3D_SDL2_FETCHED OR DUKE3D_SDL2_MIXER_FETCHED) AND DUKE3D_FETCH_SDL STREQUAL "OFF")
  message(FATAL_ERROR
    "SDL2 and/or SDL2_mixer not found and DUKE3D_FETCH_SDL=OFF. "
    "Install them (brew install sdl2 sdl2_mixer / "
    "apt install libsdl2-dev libsdl2-mixer-dev) or set DUKE3D_FETCH_SDL=AUTO.")
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

if(DUKE3D_SDL2_MIXER_FETCHED)
  message(STATUS "duke3d: building SDL2_mixer 2.8.1 from source")
  set(SDL2MIXER_INSTALL OFF CACHE BOOL "" FORCE)
  set(SDL2MIXER_SAMPLES OFF CACHE BOOL "" FORCE)
  set(SDL2MIXER_VENDORED OFF CACHE BOOL "" FORCE)
  set(SDL2MIXER_DEPS_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL2MIXER_CMD OFF CACHE BOOL "" FORCE)
  set(SDL2MIXER_FLAC OFF CACHE BOOL "" FORCE)
  set(SDL2MIXER_GME OFF CACHE BOOL "" FORCE)
  set(SDL2MIXER_MOD OFF CACHE BOOL "" FORCE)
  set(SDL2MIXER_MP3 OFF CACHE BOOL "" FORCE)
  set(SDL2MIXER_OPUS OFF CACHE BOOL "" FORCE)
  set(SDL2MIXER_VORBIS "" CACHE STRING "" FORCE)
  set(SDL2MIXER_WAVPACK OFF CACHE BOOL "" FORCE)
  set(SDL2MIXER_MIDI ON CACHE BOOL "" FORCE)
  set(SDL2MIXER_MIDI_FLUIDSYNTH OFF CACHE BOOL "" FORCE)
  set(SDL2MIXER_MIDI_TIMIDITY ON CACHE BOOL "" FORCE)
  set(SDL2MIXER_WAVE ON CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL2_mixer
    URL "${DUKE3D_SDL2_MIXER_URL}"
    URL_HASH "SHA256=${DUKE3D_SDL2_MIXER_SHA256}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  FetchContent_MakeAvailable(SDL2_mixer)
else()
  message(STATUS "duke3d: using installed SDL2_mixer ${SDL2_mixer_VERSION}")
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

add_library(duke3d_sdl2_mixer INTERFACE)
add_library(duke3d::SDL2_mixer ALIAS duke3d_sdl2_mixer)
if(TARGET SDL2_mixer::SDL2_mixer)
  target_link_libraries(duke3d_sdl2_mixer INTERFACE SDL2_mixer::SDL2_mixer)
else()
  target_link_libraries(duke3d_sdl2_mixer INTERFACE SDL2_mixer::SDL2_mixer-static)
endif()
target_link_libraries(duke3d_sdl2_mixer INTERFACE duke3d::SDL2)
