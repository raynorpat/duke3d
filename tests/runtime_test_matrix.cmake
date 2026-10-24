# Registration alone uses tiny placeholder files; no proprietary data is needed.
file(REMOVE_RECURSE "${WORK}")
foreach(_edition v099 v10 v11 v13sw v13 v14 atomic)
  file(MAKE_DIRECTORY "${WORK}/${_edition}")
  file(WRITE "${WORK}/${_edition}/DUKE3D.GRP" "fixture")
  foreach(_con GAME.CON DEFS.CON USER.CON)
    file(WRITE "${WORK}/${_edition}/${_con}" "fixture")
  endforeach()
endforeach()
file(MAKE_DIRECTORY "${WORK}/saves")
file(WRITE "${WORK}/saves/fixture.SAV" "fixture")
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${SRC}" -B "${WORK}/build"
  ${TEST_ARGS}
  "-DDUKE3D_V099_SW_DATA_DIR=${WORK}/v099" "-DDUKE3D_V10_SW_DATA_DIR=${WORK}/v10"
  "-DDUKE3D_V11_SW_DATA_DIR=${WORK}/v11" "-DDUKE3D_V13D_SW_DATA_DIR=${WORK}/v13sw"
  "-DDUKE3D_V13D_DATA_DIR=${WORK}/v13" "-DDUKE3D_V14_DATA_DIR=${WORK}/v14"
  "-DDUKE3D_ATOMIC_GRP=${WORK}/atomic/DUKE3D.GRP" "-DDUKE3D_DOS_SAVES_DIR=${WORK}/saves"
  RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
if(NOT _result EQUAL 0)
  message(FATAL_ERROR "combined fixture configure failed:\n${_out}${_err}")
endif()
file(READ "${WORK}/build/tests/CTestTestfile.cmake" _tests)
foreach(_name game_boot_v099_sw game_boot_v10_sw game_boot_v11_sw
              game_boot_v13d_sw game_boot_v13d_full game_boot_v14 game_boot_atomic
              con_compile_1.4 con_compile_1.5 savegame_codec_1.4
              net_cross_v10_v11 net_cross_13d_sw_full net_cross_13d_atomic
              con_compile_v099_sw con_compile_v10_sw con_compile_v11_sw
              con_compile_v13d_sw con_compile_v13d_full dos_save_load_fixture_SAV)
  if(NOT _tests MATCHES "add_test\\([^\n]*${_name}[^a-zA-Z0-9_]")
    message(FATAL_ERROR "combined configuration omitted ${_name}")
  endif()
endforeach()
