# Runs v13d_check_edition on DIR for EDITION (full or sw).
include("${CMAKE_CURRENT_LIST_DIR}/v13d_data.cmake")
v13d_check_edition("${DIR}" "${EDITION}")
message(STATUS "edition ok: ${EDITION}")
