# Boots a 1.3D, 1.1, 1.0 or 0.99 build headless on the original data of its
# version: to the title/demo loop, into E1L1, and (full only) into E3L1. Each
# run must still be alive when the 15 s timeout kills it. Level runs also
# require the private boot_game executable's first-tic marker and live player.
# con_compile_v13d separately checks the CONs compile cleanly.
#
#   cmake -DDUKE3D=<exe> -DDATA=<data dir> -DEDITION=<full|sw|sw11|sw10|sw099>
#         -DWORK=<scratch dir> -P game_boot_v13d.cmake
include("${CMAKE_CURRENT_LIST_DIR}/v13d_data.cmake")
v13d_check_edition("${DATA}" "${EDITION}")

# Keep WORK short: the game's config and save paths live in small buffers.
set(_run "${WORK}/run")
set(_home "${WORK}/home")
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${_run}" "${_home}")

file(CREATE_LINK "${DATA}/DUKE3D.GRP" "${_run}/duke3d.grp" SYMBOLIC COPY_ON_ERROR)
foreach(_con GAME.CON DEFS.CON USER.CON)
  file(COPY_FILE "${DATA}/${_con}" "${_run}/${_con}")
endforeach()

set(ENV{SDL_VIDEODRIVER} dummy)
# No sound: killing the game at the timeout would pop the speakers.
set(ENV{SDL_AUDIODRIVER} dummy)
set(ENV{BUILD_WINDOWED} 1)
set(ENV{HOME} "${_home}")
set(ENV{USERPROFILE} "${_home}")
set(ENV{DUKE3D_SAVE_DIR} "${_home}")

# One entry per run, arguments separated by spaces; "title" means none.
set(_runs title "/v1 /l1 /s2")
if(EDITION STREQUAL "full")
  list(APPEND _runs "/v3 /l1 /s2")
endif()

set(_failed 0)
foreach(_run_args IN LISTS _runs)
  set(_shown "${_run_args}")
  if(_run_args STREQUAL "title")
    set(_run_args "")
  else()
    string(REPLACE " " ";" _run_args "${_run_args}")
  endif()
  execute_process(COMMAND "${DUKE3D}" ${_run_args}
    WORKING_DIRECTORY "${_run}"
    TIMEOUT 15
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _out
    ERROR_VARIABLE _out)
  message(STATUS "duke3d ${_shown}: ${_result}\n${_out}")
  if(NOT _result MATCHES "timeout")
    message(SEND_ERROR "duke3d ${_shown} didn't run until the timeout: ${_result}")
    set(_failed 1)
  endif()
  if(NOT _shown STREQUAL "title")
    string(REGEX MATCH "/v([0-9]+) /l([0-9]+)" _level "${_shown}")
    if(NOT _out MATCHES "BOOTFRAME E${CMAKE_MATCH_1}L${CMAKE_MATCH_2} player=1405 health=100")
      message(SEND_ERROR "duke3d ${_shown} never completed a game tic with its live player")
      set(_failed 1)
    endif()
  endif()
endforeach()

if(_failed)
  message(FATAL_ERROR "game_boot_v13d failed")
endif()
message(STATUS "game_boot_v13d passed")
