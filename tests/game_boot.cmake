# Boots the real game headless twice: once to the title/demo loop and once
# straight into E1L1. Each run must still be alive when the 15 s timeout
# kills it. Afterwards duke3d.cfg must exist in the scratch config dir.
#
#   cmake -DDUKE3D=<exe> -DGRP=<DUKE3D.GRP> -DCON_DIR=<dir with game.con,
#         defs.con, user.con> -DWORK=<scratch dir> -DCFG_DIR=<config dir,
#         relative to the scratch HOME; "." means the run dir> -P game_boot.cmake

foreach(_var DUKE3D GRP CON_DIR WORK CFG_DIR)
  if(NOT DEFINED ${_var})
    message(FATAL_ERROR "game_boot: ${_var} is not set")
  endif()
endforeach()

# Keep WORK short: the game's config and save paths live in small buffers.
set(_run "${WORK}/run")
set(_home "${WORK}/home")
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${_run}" "${_home}")

# The game opens duke3d.grp and GAME.CON from its working directory. The
# CON files are needed there: without them it stops at a keyboard prompt.
file(CREATE_LINK "${GRP}" "${_run}/duke3d.grp" SYMBOLIC COPY_ON_ERROR)
foreach(_con GAME.CON DEFS.CON USER.CON)
  string(TOLOWER "${_con}" _src)
  file(COPY_FILE "${CON_DIR}/${_src}" "${_run}/${_con}")
endforeach()

set(ENV{SDL_VIDEODRIVER} dummy)
# No sound: killing the game at the timeout would pop the speakers.
set(ENV{SDL_AUDIODRIVER} dummy)
set(ENV{BUILD_WINDOWED} 1)
set(ENV{HOME} "${_home}")
set(ENV{USERPROFILE} "${_home}")
# Windows keeps config and saves in Saved Games; point it at the scratch home.
set(ENV{DUKE3D_SAVE_DIR} "${_home}")

set(_failed 0)
foreach(_run_args "" "/v1;/l1;/s2")
  execute_process(COMMAND "${DUKE3D}" ${_run_args}
    WORKING_DIRECTORY "${_run}"
    TIMEOUT 15
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _out
    ERROR_VARIABLE _out)
  message(STATUS "duke3d ${_run_args}: ${_result}\n${_out}")
  if(NOT _result MATCHES "timeout")
    message(SEND_ERROR "duke3d ${_run_args} didn't run until the timeout: ${_result}")
    set(_failed 1)
  endif()
  if(_out MATCHES "Not enough memory")
    message(SEND_ERROR "duke3d ${_run_args} ran out of memory")
    set(_failed 1)
  endif()
endforeach()

if(CFG_DIR STREQUAL ".")
  set(_cfg "${_run}/duke3d.cfg")
else()
  set(_cfg "${_home}/${CFG_DIR}/duke3d.cfg")
endif()
if(NOT EXISTS "${_cfg}")
  message(SEND_ERROR "no config was written: ${_cfg}")
  set(_failed 1)
endif()

if(_failed)
  message(FATAL_ERROR "game_boot failed")
endif()
message(STATUS "game_boot passed")
