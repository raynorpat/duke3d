# Windows: on first run the game copies duke3d.cfg and game*.sav from its
# working directory into the save folder. An empty GRP gets startup far
# enough with an explicit test profile; later initialization does not matter.
#
#   cmake -DDUKE3D=<exe> -DEMPTY_GRP=<empty.grp> -DWORK=<scratch dir>
#         -P save_dir.cmake

foreach(_var DUKE3D EMPTY_GRP WORK)
  if(NOT DEFINED ${_var})
    message(FATAL_ERROR "save_dir: ${_var} is not set")
  endif()
endforeach()

set(_run "${WORK}/run")
set(_saves "${WORK}/saves")
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${_run}")
file(COPY_FILE "${EMPTY_GRP}" "${_run}/duke3d.grp")
file(WRITE "${_run}/duke3d.cfg" "save_dir marker\n")
file(WRITE "${_run}/game3.sav" "save_dir marker 3\n")

set(ENV{SDL_VIDEODRIVER} dummy)
# No sound: killing the game at the timeout would pop the speakers.
set(ENV{SDL_AUDIODRIVER} dummy)
set(ENV{BUILD_WINDOWED} 1)
set(ENV{DUKE3D_SAVE_DIR} "${_saves}")
execute_process(COMMAND "${DUKE3D}" -gameversion 1.5
  WORKING_DIRECTORY "${_run}"
  TIMEOUT 15
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE _out
  ERROR_VARIABLE _out)
message(STATUS "duke3d: ${_result}\n${_out}")

set(_dir "${_saves}/Duke Nukem 3D")
if(NOT EXISTS "${_dir}/duke3d.cfg")
  message(FATAL_ERROR "duke3d.cfg wasn't copied to ${_dir}")
endif()
if(NOT EXISTS "${_dir}/game3.sav")
  message(FATAL_ERROR "game3.sav wasn't copied to ${_dir}")
endif()
# The game never writes saves at startup, so the copy must be exact.
file(READ "${_dir}/game3.sav" _got)
if(NOT _got STREQUAL "save_dir marker 3\n")
  message(FATAL_ERROR "game3.sav was copied wrong: '${_got}'")
endif()
foreach(_f duke3d.cfg game3.sav)
  if(NOT EXISTS "${_run}/${_f}")
    message(FATAL_ERROR "the original ${_f} is gone")
  endif()
endforeach()
message(STATUS "save_dir passed")
