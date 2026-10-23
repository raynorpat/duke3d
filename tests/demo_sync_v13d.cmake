# Plays one original 1.3D, 1.1 or 1.0 demo headless with -demoverify and compares the
# final state with the values measured from the DOS executable
# (tests/data/v13d_demo_refs.cmake). MODE refuse instead hands the build a
# demo with a 1.5 header, and then each demo file in OTHERS, and expects every
# one to be refused (exit 2); MODE fullonly
# hands a shareware build an episode 2 demo header and expects the refusal to
# say it needs the full version.
#
#   cmake -DDUKE3D=<exe> -DDATA=<data dir> -DEDITION=<full|sw|sw11|sw10>
#         -DDEMO=<n> -DREFS=<v13d_demo_refs.cmake> -DWORK=<scratch dir>
#         [-DMODE=refuse [-DOTHERS=<demo files>]] [-DFLAGS="<options>"]
#         -P demo_sync_v13d.cmake
#
# FLAGS adds options to the run: -demodraw draws each tic, -demoswitch <n>
# also switches renderer, display mode and resolution every n tics (and the
# run must report switches that took effect). -demoswitch runs without
# BUILD_WINDOWED, so its display modes are set (the dummy driver's display
# is no real one).
include("${CMAKE_CURRENT_LIST_DIR}/v13d_data.cmake")
v13d_check_edition("${DATA}" "${EDITION}")

set(_run "${WORK}/run")
set(_home "${WORK}/home")
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${_run}" "${_home}")

file(CREATE_LINK "${DATA}/DUKE3D.GRP" "${_run}/duke3d.grp" SYMBOLIC COPY_ON_ERROR)
foreach(_con GAME.CON DEFS.CON USER.CON)
  file(COPY_FILE "${DATA}/${_con}" "${_run}/${_con}")
endforeach()

if(MODE STREQUAL "fullonly")
  file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/data/v13d_demo_e2_header.dmo" "${_run}/demo${DEMO}.dmo")
elseif(MODE STREQUAL "refuse")
  # Any reccnt, then the 1.5 BYTEVERSION where a 1.3D demo has its volume.
  string(ASCII 1 1 1 1 117 1 1 1 1 1 1 1 _hdr)
  file(WRITE "${_run}/demo${DEMO}.dmo" "${_hdr}")
elseif(MODE STREQUAL "roundtrip")
  execute_process(COMMAND "${PYTHON}" "${CMAKE_CURRENT_LIST_DIR}/../tools/v13d/gendemo.py"
    sw099 "${_run}/demo${DEMO}.dmo" 0 0 2 260 42 RESULT_VARIABLE _gen)
  if(NOT _gen EQUAL 0)
    message(FATAL_ERROR "could not generate beta demo")
  endif()
elseif(DEMOFILE)
  file(COPY_FILE "${DEMOFILE}" "${_run}/demo${DEMO}.dmo")
else()
  file(COPY_FILE "${DATA}/DEMO${DEMO}.DMO" "${_run}/demo${DEMO}.dmo")
endif()

separate_arguments(_flags UNIX_COMMAND "${FLAGS}")

set(ENV{SDL_VIDEODRIVER} dummy)
# No sound: killing the game at the timeout would pop the speakers.
set(ENV{SDL_AUDIODRIVER} dummy)
if(FLAGS MATCHES "-demoswitch")
  unset(ENV{BUILD_WINDOWED})
else()
  set(ENV{BUILD_WINDOWED} 1)
endif()
set(ENV{HOME} "${_home}")
set(ENV{USERPROFILE} "${_home}")
set(ENV{DUKE3D_SAVE_DIR} "${_home}")

if(DEMOGOD)
  set(_demo_options -demogod)
endif()

macro(_demo_run)
  execute_process(COMMAND "${DUKE3D}" -demoverify ${DEMO} ${_flags} ${_demo_options}
    WORKING_DIRECTORY "${_run}"
    TIMEOUT 300
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _out
    ERROR_VARIABLE _err)
endmacro()
_demo_run()

if(MODE STREQUAL "fullonly")
  if(NOT _result EQUAL 2 OR NOT _out MATCHES "needs the full version")
    message(FATAL_ERROR "an episode 2 demo wasn't refused as full-version only: exit ${_result}\n${_out}${_err}")
  endif()
  message(STATUS "refused as expected")
  return()
endif()
if(MODE STREQUAL "refuse")
  set(_what "a 1.5 demo")
  set(_others ${OTHERS})
  while(TRUE)
    if(NOT _result EQUAL 2)
      message(FATAL_ERROR "${_what} wasn't refused: exit ${_result}\n${_out}${_err}")
    endif()
    if(NOT _out MATCHES "refused")
      message(FATAL_ERROR "the refusal of ${_what} printed no message:\n${_out}")
    endif()
    message(STATUS "${_what} refused as expected")
    if(NOT _others)
      break()
    endif()
    list(POP_FRONT _others _what)
    file(COPY_FILE "${_what}" "${_run}/demo${DEMO}.dmo")
    _demo_run()
  endwhile()
  return()
endif()

if(NOT _result EQUAL 0)
  message(FATAL_ERROR "duke3d -demoverify ${DEMO} failed: ${_result}\n${_out}${_err}")
endif()
if(FLAGS MATCHES "-demoswitch")
  # a switch to renderer 1 or 2, and each display mode at its size
  foreach(_want "renderer [0-9] -> [12]" "display borderless, drawn at 1024x768"
                "display exclusive, drawn at 1024x768" "display windowed, drawn at 1600x900")
    if(NOT _out MATCHES "DEMOSWITCH[^\n]*${_want}")
      message(FATAL_ERROR "-demoswitch made no switch with \"${_want}\":\n${_out}")
    endif()
  endforeach()
endif()
string(REGEX MATCH "DEMOSTATE [-0-9 ]+" _line "${_out}")
if(NOT _line)
  message(FATAL_ERROR "no DEMOSTATE line:\n${_out}")
endif()
string(REPLACE " " ";" _got "${_line}")
list(REMOVE_AT _got 0)

if(MODE STREQUAL "roundtrip")
  set(_first "${_line}")
  _demo_run()
  string(REGEX MATCH "DEMOSTATE [-0-9 ]+" _second "${_out}")
  if(NOT _result EQUAL 0 OR NOT _second STREQUAL _first)
    message(FATAL_ERROR "beta demo reader is not deterministic: ${_first} / ${_second}")
  endif()
  message(STATUS "beta demo reader repeated identically: ${_first}")
  return()
endif()

include("${REFS}")
if(REFNAME)
  set(_id "${EDITION}_${REFNAME}")
else()
  set(_id "${EDITION}_${DEMO}")
endif()
string(TOUPPER "${_id}" _id)
set(_failed 0)
set(_k 0)
foreach(_field TICS X Y Z ANG HEALTH RANDOMSEED)
  list(GET _got ${_k} _value)
  set(_want "${V13D_DEMO_${_id}_${_field}}")
  if(NOT _value STREQUAL _want)
    message(SEND_ERROR "${_field}: got ${_value}, the DOS executable has ${_want}")
    set(_failed 1)
  endif()
  math(EXPR _k "${_k} + 1")
endforeach()
if(_failed)
  message(FATAL_ERROR "demo ${EDITION} ${DEMO} is out of sync: ${_line}")
endif()
message(STATUS "demo ${EDITION} ${DEMO} in sync: ${_line}")
