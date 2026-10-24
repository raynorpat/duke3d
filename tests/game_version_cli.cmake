execute_process(
  COMMAND "${PYTHON}" -I "${CMAKE_CURRENT_LIST_DIR}/game_version_cli.py"
          --duke3d "${DUKE3D}" --work "${WORK}" ${FIXTURES}
  RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
if(NOT _result EQUAL 0)
  message(FATAL_ERROR "${_out}${_err}")
endif()
message(STATUS "${_out}")
