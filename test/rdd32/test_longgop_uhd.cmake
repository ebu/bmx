# Test creating an RDD 32 (XAVC) MXF OP1a file containing Long GOP UHD (3840x2160) essence.
# Multiple Essence Location Style is expected (body partitions), selected automatically.

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file test_longgop_uhd.mxf)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_longgop_uhd.mxf)
else()
    set(output_file test_longgop_uhd.mxf)
endif()

set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 1
    -d 15
    audio_longgop_uhd
)

set(create_command ${RAW2BMX}
    --regtest-real
    -t op1a
    --rdd32
    -o ${output_file}
    --clip test
    --avc_high_422 "${TEST_SOURCE_DIR}/longgop_uhd.h264"
    -q 16 --locked true --pcm audio_longgop_uhd
    -q 16 --locked true --pcm audio_longgop_uhd
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
    "longgop_uhd.md5"
    ""
    ""
)
