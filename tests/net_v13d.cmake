# Starts two peers over UDP on localhost and checks how they meet.
#
#   cmake -DA=<exe> -DA_DATA=<dir> -DB=<exe> -DB_DATA=<dir> -DPORT=<n>
#         -DMODE=<refuse|connect> -DWORK=<scratch dir> -P net_v13d.cmake
#
# Each data dir holds DUKE3D.GRP and GAME.CON, DEFS.CON and USER.CON. The
# peers use ports PORT and PORT+1. MODE refuse expects both peers to leave
# with the version message; MODE connect expects both to get past the
# network setup and still be running, with no version message, when the
# time runs out.
#
# The driver runs both peers at once as one execute_process pipeline of two
# "-DROLE=peer" runs of this script, each of which runs one game and keeps
# its output in a file.
if(ROLE STREQUAL "peer")
  # The UDP code seeds its player ID with time(NULL), so peers started in
  # the same second can get the same ID and give up.
  if(DELAY)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep ${DELAY})
  endif()
  set(ENV{HOME} "${RUN}/home")
  set(ENV{USERPROFILE} "${RUN}/home")
  set(ENV{DUKE3D_SAVE_DIR} "${RUN}/home")
  execute_process(COMMAND "${EXE}" -net net.cfg
    WORKING_DIRECTORY "${RUN}"
    TIMEOUT ${SECONDS}
    RESULT_VARIABLE _result
    OUTPUT_FILE "${RUN}/out.txt"
    ERROR_FILE "${RUN}/err.txt")
  file(WRITE "${RUN}/result.txt" "${_result}")
  return()
endif()

foreach(_peer A B)
  set(_run "${WORK}/${_peer}")
  file(REMOVE_RECURSE "${_run}")
  file(MAKE_DIRECTORY "${_run}/home")
  file(CREATE_LINK "${${_peer}_DATA}/DUKE3D.GRP" "${_run}/duke3d.grp" SYMBOLIC COPY_ON_ERROR)
  foreach(_con GAME.CON DEFS.CON USER.CON)
    file(COPY_FILE "${${_peer}_DATA}/${_con}" "${_run}/${_con}")
  endforeach()
endforeach()
math(EXPR _port_b "${PORT} + 1")
file(WRITE "${WORK}/A/net.cfg" "interface 127.0.0.1:${PORT}\nmode peer\nallow 127.0.0.1:${_port_b}\n")
file(WRITE "${WORK}/B/net.cfg" "interface 127.0.0.1:${_port_b}\nmode peer\nallow 127.0.0.1:${PORT}\n")

set(ENV{SDL_VIDEODRIVER} dummy)
set(ENV{SDL_AUDIODRIVER} dummy)
set(ENV{BUILD_WINDOWED} 1)

set(_seconds 20)
execute_process(
  COMMAND "${CMAKE_COMMAND}" -DROLE=peer "-DEXE=${A}" "-DRUN=${WORK}/A"
          -DSECONDS=${_seconds} -P "${CMAKE_CURRENT_LIST_FILE}"
  COMMAND "${CMAKE_COMMAND}" -DROLE=peer "-DEXE=${B}" "-DRUN=${WORK}/B"
          -DSECONDS=${_seconds} -DDELAY=2 -P "${CMAKE_CURRENT_LIST_FILE}"
  RESULT_VARIABLE _ignored)

set(_fail "")
foreach(_peer A B)
  file(READ "${WORK}/${_peer}/out.txt" _out)
  file(READ "${WORK}/${_peer}/result.txt" _result)
  string(FIND "${_out}" "You cannot play Duke with different versions" _refused)
  string(FIND "${_out}" "Multiplayer initialized" _joined)
  if(MODE STREQUAL "refuse")
    if(_refused EQUAL -1)
      string(APPEND _fail "peer ${_peer} (${${_peer}}) didn't refuse; it ended with \"${_result}\".\n")
    endif()
  else()
    if(NOT _refused EQUAL -1)
      string(APPEND _fail "peer ${_peer} (${${_peer}}) refused its own version.\n")
    elseif(_joined EQUAL -1)
      string(APPEND _fail "peer ${_peer} (${${_peer}}) never got past the network setup.\n")
    elseif(NOT _result MATCHES "timeout|terminated")
      string(APPEND _fail "peer ${_peer} (${${_peer}}) left the game early: \"${_result}\".\n")
    endif()
  endif()
endforeach()

if(_fail)
  message(FATAL_ERROR "${_fail}Peer output is in ${WORK}/A/out.txt and ${WORK}/B/out.txt.")
endif()
message(STATUS "net ${MODE}: ok")
