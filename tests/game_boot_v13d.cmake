# Boots a 1.3D build headless on the original 1.3D data: to the title/demo
# loop, into E1L1, and (full only) into E3L1. Each run must still be alive
# when the 15 s timeout kills it. (con_compile_v13d checks the CONs compile
# cleanly; the game's own output is lost when the timeout kills it.)
#
#   cmake -DDUKE3D=<exe> -DDATA=<1.3D data dir> -DEDITION=<full|sw>
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
endforeach()

if(_failed)
  message(FATAL_ERROR "game_boot_v13d failed")
endif()
message(STATUS "game_boot_v13d passed")
