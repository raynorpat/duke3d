file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
execute_process(COMMAND "${EXE}" -test-gameversion "${PROFILE}"
  WORKING_DIRECTORY "${WORK}" RESULT_VARIABLE _result
  OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
if(NOT _result EQUAL 0)
  message(FATAL_ERROR "${_out}${_err}")
endif()
