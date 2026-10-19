# Compiles the 1.3D CONs from a data folder with the game's compiler and
# compares the bytecode with the script stored in a DOS save made by the
# original executable.
#
#   cmake -DDATA=<data dir> -DEDITION=<full|sw> -DCON_COMPILE=<exe>
#         -DPYTHON=<python3> -DDECODER=<v13d_save_decode.py> -DWORK=<scratch>
#         -P con_compile_v13d.cmake
include("${CMAKE_CURRENT_LIST_DIR}/v13d_data.cmake")
v13d_check_edition("${DATA}" "${EDITION}")

set(_save "${DATA}/refsaves/e1l1start.SAV")
if(NOT EXISTS "${_save}")
  message(FATAL_ERROR "no reference save: ${_save}")
endif()

file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
foreach(_con GAME.CON DEFS.CON USER.CON)
  string(TOLOWER "${_con}" _lower)
  if(EXISTS "${DATA}/${_con}")
    set(_src "${DATA}/${_con}")
  else()
    set(_src "${DATA}/${_lower}")
  endif()
  # The compiler opens game.con, which includes DEFS.CON and USER.CON.
  if(_con STREQUAL "GAME.CON")
    file(COPY_FILE "${_src}" "${WORK}/game.con")
  else()
    file(COPY_FILE "${_src}" "${WORK}/${_con}")
  endif()
endforeach()

execute_process(COMMAND "${PYTHON}" -I "${DECODER}" script "${_save}" "${WORK}/ref.txt"
                RESULT_VARIABLE _r)
if(NOT _r EQUAL 0)
  message(FATAL_ERROR "couldn't decode ${_save}")
endif()
execute_process(COMMAND "${CON_COMPILE}" "${WORK}" "${WORK}/ref.txt"
                RESULT_VARIABLE _r)
if(NOT _r EQUAL 0)
  message(FATAL_ERROR "the compiled script doesn't match the DOS executable's")
endif()
