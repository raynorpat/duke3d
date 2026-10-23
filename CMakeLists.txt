cmake_minimum_required(VERSION 3.24)

project(duke3d LANGUAGES C)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(DUKE3D_SHAREWARE "Build the shareware (VOLUMEONE) version: 0.99, 1.0, 1.1 or 1.3D" OFF)
# Each older release needs its own data; see docs/v13d/, docs/v10-v11/ and docs/v099/.
set(DUKE3D_VERSION "1.5" CACHE STRING "Game version to build: 0.99, 1.0, 1.1, 1.3D, 1.4 or 1.5")
set_property(CACHE DUKE3D_VERSION PROPERTY STRINGS 0.99 1.0 1.1 1.3D 1.4 1.5)
string(CONCAT _version_combinations
    "Valid combinations: DUKE3D_VERSION 0.99, 1.0, 1.1, 1.3D with DUKE3D_SHAREWARE=ON; "
    "DUKE3D_VERSION 1.3D, 1.4, 1.5 with DUKE3D_SHAREWARE=OFF.")
if(NOT DUKE3D_VERSION MATCHES "^(0\\.99|1\\.0|1\\.1|1\\.3D|1\\.4|1\\.5)$")
  message(FATAL_ERROR "Unknown DUKE3D_VERSION \"${DUKE3D_VERSION}\". ${_version_combinations}")
endif()
if((DUKE3D_SHAREWARE AND NOT DUKE3D_VERSION MATCHES "^(0\\.99|1\\.0|1\\.1|1\\.3D)$") OR
   (NOT DUKE3D_SHAREWARE AND DUKE3D_VERSION MATCHES "^(0\\.99|1\\.0|1\\.1)$"))
  message(FATAL_ERROR
    "Unsupported DUKE3D_VERSION ${DUKE3D_VERSION} with DUKE3D_SHAREWARE=${DUKE3D_SHAREWARE}. "
    "${_version_combinations}")
endif()
option(DUKE3D_ROUTEBOT "Build the optional demo-authoring route bot" OFF)
option(DUKE3D_CONTROLS_MENU "Enable the in-game controls config menu" ON)
option(DUKE3D_POLYMOST "Build Ken Silverman's Polymost renderers (needs OpenGL)" ON)
set(DUKE3D_FETCH_SDL "AUTO" CACHE STRING
    "Download and build SDL2: AUTO (only if not found), ON, OFF")
set_property(CACHE DUKE3D_FETCH_SDL PROPERTY STRINGS AUTO ON OFF)

# Executables go to bin/, away from build/source/build (the engine's binary
# dir), which the "build" editor executable would otherwise collide with.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Dependencies)
include(CompileOptions)

add_library(duke3d_profile STATIC source/gameprofile.c source/gameversion.c source/gameprofile_legacy.c)
target_include_directories(duke3d_profile PUBLIC "${PROJECT_SOURCE_DIR}/source")
target_compile_features(duke3d_profile PUBLIC c_std_99)
target_link_libraries(duke3d_profile PRIVATE duke3d_compile_options duke3d_game_options)

enable_testing()

add_subdirectory(source/build)
add_subdirectory(source/audiolib)
add_subdirectory(source)
add_subdirectory(tests)
