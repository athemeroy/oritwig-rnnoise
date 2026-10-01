execute_process(COMMAND "${REFERENCE}" "${OUTPUT_DIR}/original-reference.f32"
  RESULT_VARIABLE reference_result)
execute_process(COMMAND "${ADAPTER}" "${OUTPUT_DIR}/adapter.f32"
  RESULT_VARIABLE adapter_result)
if(NOT reference_result EQUAL 0 OR NOT adapter_result EQUAL 0)
  message(FATAL_ERROR "Fixture generation failed: reference=${reference_result}, adapter=${adapter_result}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files
  "${OUTPUT_DIR}/original-reference.f32" "${OUTPUT_DIR}/adapter.f32"
  RESULT_VARIABLE comparison)
if(NOT comparison EQUAL 0)
  message(FATAL_ERROR "Original and adapter outputs differ")
endif()
file(SHA256 "${OUTPUT_DIR}/original-reference.f32" result_hash)
message(STATUS "PASS: exact original/reference output, 600 frames + VAD; SHA256=${result_hash}")
