set(log "${CMAKE_CURRENT_BINARY_DIR}/cli_test.replay")
execute_process(COMMAND "${RUNNER}" --seed 42 --ticks 100 --record "${log}"
  RESULT_VARIABLE result OUTPUT_VARIABLE recorded ERROR_VARIABLE error)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "record failed: ${error}")
endif()
execute_process(COMMAND "${RUNNER}" --replay "${log}"
  RESULT_VARIABLE result OUTPUT_VARIABLE replayed ERROR_VARIABLE error)
file(REMOVE "${log}")
if(NOT result EQUAL 0 OR NOT recorded STREQUAL replayed)
  message(FATAL_ERROR "replay mismatch: ${error}")
endif()
