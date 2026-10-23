# Configures scratch builds with bad version settings and expects each to
# stop with a message naming the valid values.
# -DSRC=<source dir> -DWORK=<scratch dir>
foreach(_args "-DDUKE3D_VERSION=1.3" "-DDUKE3D_VERSION=13d"
              "-DDUKE3D_VERSION=1.4;-DDUKE3D_SHAREWARE=ON"
              "-DDUKE3D_SHAREWARE=ON"
              "-DDUKE3D_VERSION=1.1" "-DDUKE3D_VERSION=1.0" "-DDUKE3D_VERSION=1.2"
              "-DDUKE3D_VERSION=0.99" "-DDUKE3D_VERSION=0.9")
  file(REMOVE_RECURSE "${WORK}")
  execute_process(COMMAND "${CMAKE_COMMAND}" -S "${SRC}" -B "${WORK}" ${_args}
                  RESULT_VARIABLE _r OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
  if(_r EQUAL 0)
    message(FATAL_ERROR "configure with ${_args} succeeded")
  endif()
  string(REGEX REPLACE "[ \r\n]+" " " _diagnostic "${_out}${_err}")
  if(NOT _diagnostic MATCHES "0\\.99, 1\\.0, 1\\.1, 1\\.3D|0\\.99.*shareware")
    message(FATAL_ERROR "configure with ${_args} failed without naming the valid values:\n${_err}")
  endif()
endforeach()
file(REMOVE_RECURSE "${WORK}")
