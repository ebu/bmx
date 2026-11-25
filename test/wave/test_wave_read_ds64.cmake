# Test reading an RF64 Wave file with a ds64 chunk larger than 28 bytes.
# ds64_64.wav has a 64-byte ds64 chunk: the 28-byte base fields, a chunk size table
# with 1 entry and 24 reserved bytes, as allowed by EBU Tech 3306 / ITU-R BS.2088.

set(test_name wave_read_ds64)
include("${TEST_SOURCE_DIR}/test_common.cmake")

set(create_command ${RAW2BMX}
    --regtest
    -t wave
    -o ${output_file}
    --wave ${TEST_SOURCE_DIR}/ds64_64.wav
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    ""
    ""
    ""
    "${create_command}"
    ""
    ""
    ""
    "${output_file}"
    "wave_read_ds64.md5"
    ""
    ""
)
