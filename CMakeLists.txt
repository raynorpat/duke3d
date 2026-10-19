cmake_minimum_required(VERSION 3.24)

project(duke3d LANGUAGES C)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(DUKE3D_SHAREWARE "Build the shareware (VOLUMEONE) version" OFF)
# Which release's game logic to build. 1.3D and 1.4 need their own data; see
# docs/v13d/reference.md.
set(DUKE3D_VERSION "1.5" CACHE STRING "Game version to build: 1.3D, 1.4 or 1.5")
set_property(CACHE DUKE3D_VERSION PROPERTY STRINGS 1.3D 1.4 1.5)
if(NOT DUKE3D_VERSION MATCHES "^(1\\.3D|1\\.4|1\\.5)$")
  message(FATAL_ERROR
    "DUKE3D_VERSION is \"${DUKE3D_VERSION}\"; it must be one of 1.3D, 1.4, 1.5")
endif()
if(DUKE3D_VERSION STREQUAL "1.4" AND DUKE3D_SHAREWARE)
  message(FATAL_ERROR
    "1.4 has no shareware build; use DUKE3D_VERSION 1.3D, 1.4, 1.5 without "
    "DUKE3D_SHAREWARE for 1.4")
endif()
option(DUKE3D_CONTROLS_MENU "Enable the in-game controls config menu" ON)
set(DUKE3D_FETCH_SDL "AUTO" CACHE STRING
    "Download and build SDL2: AUTO (only if not found), ON, OFF")
set_property(CACHE DUKE3D_FETCH_SDL PROPERTY STRINGS AUTO ON OFF)

# Executables go to bin/, away from build/source/build (the engine's binary
# dir), which the "build" editor executable would otherwise collide with.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Dependencies)
include(CompileOptions)

enable_testing()

add_subdirectory(source/build)
add_subdirectory(source/audiolib)
add_subdirectory(source)
add_subdirectory(tests)
