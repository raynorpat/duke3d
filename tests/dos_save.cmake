# Runs duke3d headless on a DOS 1.5 save. MODE is one of:
#   load       -loadverify must print the SAVESTATE the Python decoder prints
#   roundtrip  load, save again with -saveverify, then compare field by field
#   refuse     an old-layout copy and a version-103 copy must both exit 2
#   hostile    a copy with out-of-range counts must exit 0, 1 or 2, not crash
#
#   cmake -DMODE=<mode> -DDUKE3D=<exe> -DGRP=<Atomic DUKE3D.GRP> -DSAVE=<.sav>
#         -DPYTHON=<python3> -DDECODER=<dos_save_decode.py> -DWORK=<scratch dir>
#         -P dos_save.cmake

foreach(_var MODE DUKE3D GRP SAVE PYTHON DECODER WORK)
  if(NOT DEFINED ${_var})
    message(FATAL_ERROR "dos_save: ${_var} is not set")
  endif()
endforeach()

# Keep WORK short: the game's config and save paths live in small buffers.
set(_run "${WORK}/run")
set(_home "${WORK}/home")
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${_run}" "${_home}")

# The game opens duke3d.grp and the CON files from its working directory.
# The extracted Atomic CONs sit next to the GRP.
file(CREATE_LINK "${GRP}" "${_run}/duke3d.grp" SYMBOLIC COPY_ON_ERROR)
get_filename_component(_grp_dir "${GRP}" DIRECTORY)
foreach(_con GAME.CON DEFS.CON USER.CON)
  if(EXISTS "${_grp_dir}/${_con}")
    file(COPY_FILE "${_grp_dir}/${_con}" "${_run}/${_con}")
  endif()
endforeach()
# A relative name: the game treats an argument starting with / as a switch.
file(COPY_FILE "${SAVE}" "${_run}/dos.sav")

set(ENV{SDL_VIDEODRIVER} dummy)
set(ENV{BUILD_WINDOWED} 1)
set(ENV{HOME} "${_home}")
set(ENV{USERPROFILE} "${_home}")

function(run_duke3d result output)
  execute_process(COMMAND "${DUKE3D}" ${ARGN}
    WORKING_DIRECTORY "${_run}"
    TIMEOUT 60
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _out
    ERROR_VARIABLE _out)
  message(STATUS "duke3d ${ARGN}: ${_result}\n${_out}")
  set(${result} "${_result}" PARENT_SCOPE)
  set(${output} "${_out}" PARENT_SCOPE)
endfunction()

function(run_decoder output)
  execute_process(COMMAND "${PYTHON}" -I "${DECODER}" ${ARGN}
    WORKING_DIRECTORY "${_run}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _out
    ERROR_VARIABLE _out)
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR "dos_save_decode.py ${ARGN} failed (${_result}):\n${_out}")
  endif()
  string(STRIP "${_out}" _out)
  set(${output} "${_out}" PARENT_SCOPE)
endfunction()

if(MODE STREQUAL "load")
  run_duke3d(_result _out -loadverify dos.sav)
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR "-loadverify exited ${_result}")
  endif()
  string(REGEX MATCH "SAVESTATE[^\r\n]*" _got "${_out}")
  run_decoder(_want state dos.sav)
  if(NOT _got STREQUAL _want)
    message(FATAL_ERROR "game: '${_got}'\ndecoder: '${_want}'")
  endif()
elseif(MODE STREQUAL "roundtrip")
  run_duke3d(_result _out -loadverify dos.sav -saveverify out.sav)
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR "-loadverify -saveverify exited ${_result}")
  endif()
  run_decoder(_diff compare dos.sav out.sav)
elseif(MODE STREQUAL "refuse")
  run_decoder(_ignored fixture oldlayout dos.sav old.sav)
  run_decoder(_ignored fixture version dos.sav v103.sav)
  foreach(_case "old.sav;not a DOS 1.5 save" "v103.sav;is a version 103 save")
    list(GET _case 0 _file)
    list(GET _case 1 _msg)
    run_duke3d(_result _out -loadverify ${_file})
    if(NOT _result EQUAL 2)
      message(FATAL_ERROR "${_file}: -loadverify exited ${_result}, not 2")
    endif()
    if(NOT _out MATCHES "${_msg}")
      message(FATAL_ERROR "${_file}: no '${_msg}' message")
    endif()
  endforeach()
elseif(MODE STREQUAL "hostile")
  run_decoder(_ignored fixture bigcounts dos.sav big.sav)
  run_duke3d(_result _out -loadverify big.sav)
  if(NOT _result MATCHES "^[0-9]+$")
    message(FATAL_ERROR "big.sav: -loadverify ended with '${_result}'")
  endif()
else()
  message(FATAL_ERROR "dos_save: unknown MODE ${MODE}")
endif()
message(STATUS "dos_save ${MODE} passed")
