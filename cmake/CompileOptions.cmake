# Flags and definitions shared by every duke3d target. Link
# duke3d_compile_options to pick them up.

include(CheckCCompilerFlag)

add_library(duke3d_compile_options INTERFACE)

target_compile_definitions(duke3d_compile_options INTERFACE USE_SDL=1)

if(WIN32)
  target_compile_definitions(duke3d_compile_options INTERFACE
    PLATFORM_WIN32=1)
elseif(APPLE)
  target_compile_definitions(duke3d_compile_options INTERFACE
    PLATFORM_UNIX=1 PLATFORM_MACOSX=1)
else()
  target_compile_definitions(duke3d_compile_options INTERFACE
    PLATFORM_UNIX=1)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(duke3d_compile_options INTERFACE
      USE_EXECINFO=1)
  endif()
endif()

if(MSVC)
  target_compile_definitions(duke3d_compile_options INTERFACE
    _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
  # /J: char is unsigned, which the game logic assumes.
  target_compile_options(duke3d_compile_options INTERFACE /J /W3 /wd4996)
else()
  # The game logic assumes unsigned char, and the code relies on
  # type-punning that breaks under strict aliasing.
  target_compile_options(duke3d_compile_options INTERFACE
    -funsigned-char -fno-strict-aliasing -W -Wall -Wno-unused)

  # Newer compilers turn these 1990s-isms into hard errors; keep them as
  # warnings so the original code builds unmodified.
  foreach(_flag
      implicit-function-declaration
      implicit-int
      int-conversion
      incompatible-pointer-types
      return-mismatch)
    string(MAKE_C_IDENTIFIER "DUKE3D_HAS_WNO_ERROR_${_flag}" _var)
    check_c_compiler_flag("-Wno-error=${_flag}" ${_var})
    if(${_var})
      target_compile_options(duke3d_compile_options INTERFACE
        "-Wno-error=${_flag}")
    endif()
  endforeach()
endif()

target_compile_features(duke3d_compile_options INTERFACE c_std_99)

# Game-specific definitions, for duke3d and the build editor only.
add_library(duke3d_game_options INTERFACE)
if(DUKE3D_SHAREWARE)
  target_compile_definitions(duke3d_game_options INTERFACE VOLUMEONE)
else()
  target_compile_definitions(duke3d_game_options INTERFACE VOLUMEALL)
endif()
if(DUKE3D_CONTROLS_MENU)
  target_compile_definitions(duke3d_game_options INTERFACE
    CONTROLS_CONFIG_MENU=1)
endif()
