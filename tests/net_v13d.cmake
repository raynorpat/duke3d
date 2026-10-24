# Starts two peers over UDP on localhost and checks how they meet.
#
#   cmake -DA=<exe> -DA_DATA=<dir> -DB=<exe> -DB_DATA=<dir> -DPORT=<n>
#         -DMODE=<refuse|connect> -DWORK=<scratch dir> [-DARGS="<options>"]
#         [-DA_ARGS="<options>" -DA_EXPECT=<regex>] -P net_v13d.cmake
#
# A is the net_game test wrapper, which flushes transport readiness to stdout.
# ARGS, space-separated, go to both peers after "-net net.cfg"; "/v1 /l1"
# puts them straight into a level, so the game runs and checks its sync.
# A_ARGS go to peer A only, and A_EXPECT must then match its output.
#
# Each data dir holds DUKE3D.GRP and GAME.CON, DEFS.CON and USER.CON. The
# peers use ports PORT and PORT+1. MODE refuse expects both peers to leave
# with the version message; MODE connect expects both to get past the
# network setup and still be running, with no version message and no
# out-of-sync report, when the time runs out. Both must have compared sync values.
# B_VERSION instead selects a data-free protocol refusal fixture for peer B.
#
# The driver runs both peers at once as one execute_process pipeline of two
# "-DROLE=peer" runs of this script, each of which runs one game and keeps
# its output in a file.
if(ROLE STREQUAL "peer")
  # srand(time(NULL)) precedes the first peer's ID line. Wait for that line
  # before crossing a second boundary, so slow startup cannot collide IDs.
  if(WAIT_FOR)
    string(TIMESTAMP _started "%s")
    while(1)
      if(EXISTS "${WAIT_FOR}/out.txt")
        file(READ "${WAIT_FOR}/out.txt" _ready)
        if(_ready MATCHES "This client's ID for this round")
          break()
        endif()
      endif()
      string(TIMESTAMP _now "%s")
      math(EXPR _elapsed "${_now} - ${_started}")
      if(_elapsed GREATER_EQUAL SECONDS OR EXISTS "${WAIT_FOR}/result.txt")
        file(WRITE "${RUN}/out.txt" "First peer never reached transport readiness.\n")
        file(WRITE "${RUN}/result.txt" "not started")
        return()
      endif()
      execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 0.1)
    endwhile()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1.1)
  endif()
  set(ENV{HOME} "${RUN}/home")
  set(ENV{USERPROFILE} "${RUN}/home")
  set(ENV{DUKE3D_SAVE_DIR} "${RUN}/home")
  separate_arguments(_args UNIX_COMMAND "${ARGS}")
  execute_process(COMMAND "${EXE}" -net net.cfg ${_args}
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
  if(_peer STREQUAL "B" AND B_VERSION)
    continue()
  endif()
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
set(_b_args "${ARGS}")
if(B_VERSION)
  set(_b_args "${B_VERSION}")
  if(MODE STREQUAL "maprefuse")
    set(_b_args "-test-custom-map ${B_VERSION}")
  endif()
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" -DROLE=peer "-DEXE=${A}" "-DRUN=${WORK}/A"
          -DSECONDS=${_seconds} "-DARGS=${ARGS} ${A_ARGS}" -P "${CMAKE_CURRENT_LIST_FILE}"
  COMMAND "${CMAKE_COMMAND}" -DROLE=peer "-DEXE=${B}" "-DRUN=${WORK}/B"
          -DSECONDS=${_seconds} "-DWAIT_FOR=${WORK}/A" "-DARGS=${_b_args} ${B_ARGS}" -P "${CMAKE_CURRENT_LIST_FILE}"
  RESULT_VARIABLE _ignored)

set(_fail "")
foreach(_peer A B)
  file(READ "${WORK}/${_peer}/out.txt" _out)
  file(READ "${WORK}/${_peer}/result.txt" _result)
  string(FIND "${_out}" "You cannot play Duke with different versions" _refused)
  string(FIND "${_out}" "Multiplayer initialized" _joined)
  string(FIND "${_out}" "Multiplayer sync checked." _checked)
  string(FIND "${_out}" "Out Of Sync" _oos)
  if(MODE STREQUAL "maprefuse")
    if(_peer STREQUAL "A" AND NOT _out MATCHES "Shareware profiles cannot load custom network maps")
      string(APPEND _fail "shareware peer did not refuse the custom map request.\n")
    elseif(_peer STREQUAL "B" AND NOT _out MATCHES "Custom map request sent")
      string(APPEND _fail "map request peer never sent its request.\n")
    endif()
  elseif(MODE STREQUAL "refuse")
    if(_refused EQUAL -1)
      string(APPEND _fail "peer ${_peer} (${${_peer}}) didn't refuse; it ended with \"${_result}\".\n")
    endif()
    if(_peer STREQUAL "B" AND B_VERSION AND NOT _out MATCHES "Protocol peer exchanged BYTEVERSION ${B_VERSION}")
      string(APPEND _fail "protocol peer never exchanged its version.\n")
    endif()
  else()
    if(NOT _refused EQUAL -1)
      string(APPEND _fail "peer ${_peer} (${${_peer}}) refused its own version.\n")
    elseif(_joined EQUAL -1)
      string(APPEND _fail "peer ${_peer} (${${_peer}}) never got past the network setup.\n")
    elseif(NOT _oos EQUAL -1)
      string(APPEND _fail "peer ${_peer} (${${_peer}}) went out of sync.\n")
    elseif(_checked EQUAL -1)
      string(APPEND _fail "peer ${_peer} (${${_peer}}) never compared a sync value.\n")
    elseif(NOT _result MATCHES "timeout|terminated")
      string(APPEND _fail "peer ${_peer} (${${_peer}}) left the game early: \"${_result}\".\n")
    elseif(_peer STREQUAL "A" AND A_EXPECT AND NOT _out MATCHES "${A_EXPECT}")
      string(APPEND _fail "peer A's output doesn't show \"${A_EXPECT}\".\n")
    endif()
  endif()
endforeach()

if(_fail)
  message(FATAL_ERROR "${_fail}Peer output is in ${WORK}/A/out.txt and ${WORK}/B/out.txt.")
endif()
message(STATUS "net ${MODE}: ok")
