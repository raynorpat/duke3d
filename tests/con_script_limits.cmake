# Literal native compiler limits are independent of production profile helpers.
foreach(_row "0.99;20000" "1.0;20000" "1.1;20000" "1.3d-shareware;20000"
             "1.3d;20000" "1.4;20460" "1.5;20460")
  list(GET _row 0 _profile)
  list(GET _row 1 _limit)
  foreach(_delta -8 8)
    math(EXPR _words "${_limit}+${_delta}")
    execute_process(COMMAND "${EXE}" -test-gameversion "${_profile}" "${WORK}/limit.con" ${_words}
      RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
    if(_delta LESS 0)
      if(NOT _result EQUAL 0 OR NOT _out MATCHES "Compiled limit probe")
        message(FATAL_ERROR "${_profile} rejected an in-range script:\n${_out}${_err}")
      endif()
    elseif(_result EQUAL 0 OR NOT "${_out}${_err}" MATCHES "Script exceeds selected profile capacity")
      message(FATAL_ERROR "${_profile} accepted an oversized script:\n${_out}${_err}")
    endif()
  endforeach()
endforeach()
