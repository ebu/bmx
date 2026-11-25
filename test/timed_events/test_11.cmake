# Test roundtripping the timed events manifest. Write the same MXF file a second time
# using the timed events manifest extracted from the first written file.

set(test_num 11)
include("${TEST_SOURCE_DIR}/test_common.cmake")

set(create_command_1 ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -y 09:59:59:24
    -o ${output_file}
    --timed-events "${TEST_SOURCE_DIR}/manifest_4.xml"
    --avci100_1080i video_1_${test_num}
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_video_1}"
    ""
    ""
    "${create_command_1}"
    ""
    ""
    "${read_command}"
    "${output_file}"
    "test_11.md5"
    "${output_info_file};info_11.xml.bin"
    ""
)

set(create_command_2 ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -y 09:59:59:24
    -o ${output_file}
    --timed-events "${output_essence_file_prefix}_d0_manifest.xml"
    --avci100_1080i video_1_${test_num}
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_video_1}"
    ""
    ""
    "${create_command_2}"
    ""
    ""
    "${read_command}"
    "${output_file}"
    "test_11.md5"
    "${output_info_file};info_11.xml.bin"
    ""
)
