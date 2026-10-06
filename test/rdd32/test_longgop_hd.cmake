# Test creating an RDD 32 (XAVC) MXF OP1a file containing Long GOP HD essence.
# Multiple Essence Location Style is expected (body partitions), selected automatically.

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file test_longgop_hd.mxf)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_longgop_hd.mxf)
else()
    set(output_file test_longgop_hd.mxf)
endif()

set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 1
    -d 15
    audio_longgop_hd
)

set(create_command ${RAW2BMX}
    --regtest-real
    -t op1a
    --rdd32
    -o ${output_file}
    --clip test
    --avc_high_422 "${TEST_SOURCE_DIR}/../avc_separate_fields/separate_fields_gop.h264"
    -q 16 --locked true --pcm audio_longgop_hd
    -q 16 --locked true --pcm audio_longgop_hd
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    ""
    "${create_test_audio}"
    ""
    "${create_command}"
    ""
    ""
    ""
    "${output_file}"
    "longgop_hd.md5"
    ""
    ""
)
