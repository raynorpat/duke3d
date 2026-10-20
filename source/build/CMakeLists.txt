# Ken Silverman's Build engine, as ported by icculus. See VENDORED.md.

add_library(buildengine STATIC
  a.c
  cache1d.c
  engine.c
  mmulti.c
  pragmas.c
  sdl_driver.c)

if(WIN32)
  target_sources(buildengine PRIVATE win32_compat.c)
else()
  target_sources(buildengine PRIVATE unix_compat.c)
endif()

target_include_directories(buildengine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(buildengine PRIVATE UDP_NETWORKING=1)

if(NOT MSVC)
  target_compile_options(buildengine PRIVATE -fno-omit-frame-pointer)
endif()

target_link_libraries(buildengine PUBLIC duke3d_compile_options duke3d::SDL2)

# engine.c #includes polymost.c, so it's listed only for IDEs.
target_sources(buildengine PRIVATE polymost.c)
set_source_files_properties(polymost.c PROPERTIES HEADER_FILE_ONLY ON)
if(DUKE3D_POLYMOST)
  find_package(OpenGL REQUIRED)
  target_compile_definitions(buildengine PUBLIC POLYMOST USE_OPENGL)
  if(APPLE)
    target_compile_definitions(buildengine PUBLIC GL_SILENCE_DEPRECATION)
  endif()
  target_link_libraries(buildengine PUBLIC OpenGL::GL)
endif()

if(WIN32)
  target_link_libraries(buildengine PUBLIC ws2_32 winmm)
elseif(NOT APPLE)
  target_link_libraries(buildengine PUBLIC m)
endif()
