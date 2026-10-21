# Runs a 1.3D or 1.1 build headless on a DOS save of its own release. MODE is
# one of:
#   load    -loadverify must print the reference state (tests/data/v13d_save_refs.cmake);
#           then -saveverify writes the game back out. That copy must hold the same
#           blocks as the DOS save (v13d_save_decode.py compare), and loading it must
#           print the same state
#   refuse  each OTHER save (another release or edition) and one with the 1.5
#           BYTEVERSION must exit 2 with the version message
#
#   cmake -DMODE=<mode> -DDUKE3D=<exe> -DDATA=<1.3D data dir> -DEDITION=<full|sw|sw11>
#         -DREFS=<v13d_save_refs.cmake> -DPYTHON=<python3> -DDECODER=<v13d_save_decode.py>
#         -DWORK=<scratch dir>
#         [-DNAME=<ref name>] [-DOTHER=<other releases' saves, ;-separated>] -P dos_save_v13d.cmake
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
set(ENV{SDL_AUDIODRIVER} dummy)
set(ENV{BUILD_WINDOWED} 1)
set(ENV{HOME} "${_home}")
set(ENV{USERPROFILE} "${_home}")
set(ENV{DUKE3D_SAVE_DIR} "${_home}")

# Save names are relative: the game treats an argument starting with / as a switch.
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

if(MODE STREQUAL "load")
  include("${REFS}")
  set(_p "V13D_SAVE_${NAME}")
  file(COPY_FILE "${DATA}/refsaves/${${_p}_FILE}" "${_run}/dos.sav")
  string(REGEX MATCH "^E([0-9]+)L([0-9]+)$" _ "${${_p}_LEVEL}")
  math(EXPR _vol "${CMAKE_MATCH_1} - 1")
  math(EXPR _lev "${CMAKE_MATCH_2} - 1")
  # health armor curweapon ammo volume level x y
  set(_want "SAVESTATE ${${_p}_HEALTH} ${${_p}_ARMOR} ${${_p}_CURWEAPON} ${${_p}_AMMO} ${_vol} ${_lev} ${${_p}_X} ${${_p}_Y}")

  run_duke3d(_result _out -loadverify dos.sav -saveverify ours.sav)
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR "-loadverify dos.sav exited ${_result}")
  endif()
  string(REGEX MATCH "SAVESTATE[^\r\n]*" _got "${_out}")
  if(NOT _got STREQUAL _want)
    message(FATAL_ERROR "DOS save\n game: '${_got}'\n want: '${_want}'")
  endif()

  execute_process(COMMAND "${PYTHON}" -I "${DECODER}" compare dos.sav ours.sav
    WORKING_DIRECTORY "${_run}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _out
    ERROR_VARIABLE _out)
  message(STATUS "compare dos.sav ours.sav: ${_result}\n${_out}")
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR "our save of it doesn't match the DOS save")
  endif()

  run_duke3d(_result _out -loadverify ours.sav)
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR "-loadverify ours.sav exited ${_result}")
  endif()
  string(REGEX MATCH "SAVESTATE[^\r\n]*" _got "${_out}")
  if(NOT _got STREQUAL _want)
    message(FATAL_ERROR "our save of it\n game: '${_got}'\n want: '${_want}'")
  endif()
elseif(MODE STREQUAL "refuse")
  set(_files "")
  set(_n 0)
  foreach(_other IN LISTS OTHER)
    math(EXPR _n "${_n} + 1")
    file(COPY_FILE "${_other}" "${_run}/other${_n}.sav")
    list(APPEND _files other${_n}.sav)
  endforeach()
  # A 1.5 save's first block: BYTEVERSION 117, uncompressed.
  file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/data/v15_save_header.sav" "${_run}/v15.sav")
  list(APPEND _files v15.sav)
  foreach(_file IN LISTS _files)
    run_duke3d(_result _out -loadverify ${_file})
    if(NOT _result EQUAL 2)
      message(FATAL_ERROR "${_file}: -loadverify exited ${_result}, not 2")
    endif()
    # A 1.0 save has no version: its first four name bytes are read as one.
    if(NOT _out MATCHES "is a version -?[0-9]+ save, not")
      message(FATAL_ERROR "${_file}: no version message")
    endif()
  endforeach()
else()
  message(FATAL_ERROR "dos_save_v13d: unknown MODE ${MODE}")
endif()
message(STATUS "dos_save_v13d ${MODE} passed")
