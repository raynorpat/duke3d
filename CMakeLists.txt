cmake_minimum_required(VERSION 3.24)

project(duke3d LANGUAGES C)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

if(DEFINED DUKE3D_VERSION OR DEFINED DUKE3D_SHAREWARE)
  message(FATAL_ERROR
    "DUKE3D_VERSION and DUKE3D_SHAREWARE are obsolete. Remove them from the command and cache "
    "(cmake -U DUKE3D_VERSION -U DUKE3D_SHAREWARE ...). Releases are detected from the base GRP; "
    "use -gameversion TOKEN at runtime for modified GRPs.")
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

add_library(duke3d_profile STATIC source/gameprofile.c source/gameversion.c)
target_include_directories(duke3d_profile PUBLIC "${PROJECT_SOURCE_DIR}/source")
target_compile_features(duke3d_profile PUBLIC c_std_99)
target_link_libraries(duke3d_profile PRIVATE duke3d_compile_options)

enable_testing()

add_subdirectory(source/build)
add_subdirectory(source/audiolib)
add_subdirectory(source)
add_subdirectory(tests)
