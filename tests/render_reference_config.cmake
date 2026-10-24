# Reference policy and cache migration must work without commercial data.
include("${CMAKE_CURRENT_LIST_DIR}/render_references.cmake")
function(expect profile key expected)
  duke_render_reference("${profile}" "${key}" FALSE _actual)
  if(NOT _actual STREQUAL expected)
    message(FATAL_ERROR "${profile}/${key}: ${_actual}, expected ${expected}")
  endif()
endfunction()
expect(1.3d E1L1 e0517697)
expect(1.5 E1L1 99d777cd)
expect(1.3d E1L1_TILT 64e7f538)
expect(1.5 E1L1_TILT 67030c2c)
expect(1.3d CLASSIC2D_640x400_MAP 7282abe5)
expect(1.5 CLASSIC2D_640x400_MAP b08ff175)
expect(unknown E1L1 "")
expect(unsupported E1L1 "")
expect(1.4 E1L1 "")
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/data/empty.grp" "${WORK}/unknown.grp")

function(query grp settings generic version expected)
  execute_process(COMMAND "${CMAKE_COMMAND}" "-DIDENTIFY=${IDENTIFY}" "-DGRP=${grp}"
    "-DSETTINGS=${settings}" "-DGENERIC=${generic}" "-DGAMEVERSION=${version}"
    -DKIND=normal -DQUERY_ONLY=ON -P "${CMAKE_CURRENT_LIST_DIR}/render_reference_run.cmake"
    RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
  if(NOT _result EQUAL 0 OR NOT "${_out}${_err}" MATCHES "${expected}")
    message(FATAL_ERROR "reference query failed: ${_result}\n${_out}${_err}\nexpected ${expected}")
  endif()
endfunction()
query("${WORK}/unknown.grp" "" FALSE "" "profile=unknown reference1=<> reference2=<>")
execute_process(COMMAND "${IDENTIFY}" --identify "${CMAKE_CURRENT_LIST_FILE}"
  RESULT_VARIABLE _bad OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
if(_bad EQUAL 0)
  message(FATAL_ERROR "invalid GRP was identified successfully")
endif()

# Configure once clean, then reuse the same cache with old default pins.
function(configure)
  execute_process(COMMAND "${CMAKE_COMMAND}" -S "${SRC}" -B "${WORK}/build"
    ${TEST_ARGS} ${ARGN} RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR "reference configure failed:\n${_out}${_err}")
  endif()
endfunction()
configure()
set(_settings "${WORK}/build/tests/render_reference_settings.cmake")
include("${_settings}")
if(NOT DUKE3D_RENDER_GENERIC_E1L1 STREQUAL "stock")
  message(FATAL_ERROR "clean generic references should follow fingerprint identity")
endif()
file(APPEND "${WORK}/build/CMakeCache.txt"
  "\nDUKE3D_E1L1_CHECKSUM:STRING=e0517697\nDUKE3D_E1L1_TILT_CHECKSUM:STRING=64e7f538\nDUKE3D_CLASSIC2D_640x400_MAP:STRING=7282abe5\n")
configure()
include("${_settings}")
expect(1.5 E1L1 99d777cd)
expect(1.5 E1L1_TILT 67030c2c)
expect(1.5 CLASSIC2D_640x400_MAP b08ff175)
if(NOT DUKE3D_RENDER_GENERIC_E1L1 STREQUAL "stock")
  message(FATAL_ERROR "old cached 1.3D default became a generic override")
endif()
# Nondefault legacy overrides migrate only to the generic test case.
configure(-UDUKE3D_RENDER_GENERIC_E1L1 -DDUKE3D_E1L1_CHECKSUM=12345678)
include("${_settings}")
duke_render_reference(1.5 E1L1 TRUE _generic)
if(NOT _generic STREQUAL "12345678")
  message(FATAL_ERROR "legacy explicit override was lost")
endif()
expect(1.5 E1L1 99d777cd)
configure(-DDUKE3D_RENDER_ATOMIC_E1L1=abcdef12)
include("${_settings}")
expect(1.5 E1L1 abcdef12)
expect(1.3d E1L1 e0517697)
query("${WORK}/unknown.grp" "${_settings}" TRUE 1.5 "profile=unknown reference1=<12345678>")

foreach(_edition 13d atomic)
  if(EXISTS "${GRP_${_edition}}")
    if(_edition STREQUAL "13d")
      set(_token 1.3d)
      set(_sum e0517697)
    else()
      set(_token 1.5)
      set(_sum 99d777cd)
    endif()
    file(CREATE_LINK "${GRP_${_edition}}" "${WORK}/renamed_${_edition}.dat" SYMBOLIC COPY_ON_ERROR)
    query("${WORK}/renamed_${_edition}.dat" "" FALSE "" "profile=${_token} reference1=<${_sum}>")
    file(COPY_FILE "${GRP_${_edition}}" "${WORK}/modified.grp")
    file(APPEND "${WORK}/modified.grp" "modified")
    query("${WORK}/modified.grp" "" FALSE "" "profile=unknown reference1=<> reference2=<>")
    foreach(_kind normal tilt)
      execute_process(COMMAND "${CMAKE_COMMAND}" "-DIDENTIFY=${IDENTIFY}"
        "-DEXE=${RENDER}" "-DGRP=${WORK}/modified.grp" "-DOUT=${WORK}/modified.bmp"
        "-DGAMEVERSION=${_token}" "-DKIND=${_kind}" -DSIZE=320x200
        -P "${CMAKE_CURRENT_LIST_DIR}/render_reference_run.cmake"
        RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
      if(NOT _result EQUAL 0 OR NOT "${_out}${_err}" MATCHES "No stock golden coverage")
        message(FATAL_ERROR "modified data inherited an assertion:\n${_out}${_err}")
      endif()
    endforeach()
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DIDENTIFY=${IDENTIFY}"
      "-DEXE=${MINIHUD}" "-DGRP=${WORK}/modified.grp" "-DOUT=${WORK}/mini"
      "-DGAMEVERSION=${_token}" -DKIND=minihud -DSIZE=320x200
      -DMINIREF1=00000001 -DMINIREF2=00000001 -DMINIREF3=00000001
      -P "${CMAKE_CURRENT_LIST_DIR}/render_reference_run.cmake"
      RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
    if(NOT _result EQUAL 0 OR NOT "${_out}${_err}" MATCHES "No stock golden coverage")
      message(FATAL_ERROR "unknown minihud asserted stock pins:\n${_out}${_err}")
    endif()
    # Verify the wrapper really forwards a user assertion to the renderer.
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DIDENTIFY=${IDENTIFY}"
      "-DEXE=${RENDER}" "-DGRP=${GRP_${_edition}}" "-DOUT=${WORK}/override.bmp"
      "-DSETTINGS=${_settings}" -DGENERIC=TRUE -DKIND=normal
      -P "${CMAKE_CURRENT_LIST_DIR}/render_reference_run.cmake"
      RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
    if(_result EQUAL 0 OR NOT "${_out}${_err}" MATCHES "12345678")
      message(FATAL_ERROR "explicit checksum was not asserted:\n${_out}${_err}")
    endif()
  endif()
endforeach()
