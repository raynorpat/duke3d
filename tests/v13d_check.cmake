# Runs v13d_check_edition on DIR for EDITION (full, sw, sw11 or sw10).
# With OTHERS, a list of folders, it instead expects each of them to be
# refused as the wrong edition.
include("${CMAKE_CURRENT_LIST_DIR}/v13d_data.cmake")
if(DEFINED OTHERS)
  foreach(_dir IN LISTS OTHERS)
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DDIR=${_dir}" "-DEDITION=${EDITION}"
                            -P "${CMAKE_CURRENT_LIST_FILE}"
                    RESULT_VARIABLE _r OUTPUT_VARIABLE _out ERROR_VARIABLE _out)
    if(_r EQUAL 0 OR NOT _out MATCHES "wrong edition: expected ${EDITION}")
      message(FATAL_ERROR "${_dir} wasn't refused as the wrong edition:\n${_out}")
    endif()
    message(STATUS "refused: ${_dir}")
  endforeach()
  return()
endif()
v13d_check_edition("${DIR}" "${EDITION}")
message(STATUS "edition ok: ${EDITION}")
