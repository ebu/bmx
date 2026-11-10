# Test creating an RDD 32 (XAVC) MXF OP1a file containing XAVC Long GOP essence
# (AVC High 4:2:2, 1080p50, 50 Mbit/s, GOP size 24, 4 slices per frame).
# Multiple Essence Location Style is expected (body partitions), selected automatically.
# The essence covers 2 complete GOPs (the first is 22 frames due to encoder lookahead, the
# second is a full 24-frame GOP) plus the start of a third GOP, confirming SPS/PPS repeat
# correctly at GOP boundaries.

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file test_longgop.mxf)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_longgop.mxf)
else()
    set(output_file test_longgop.mxf)
endif()

set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 1
    -d 60
    audio_longgop
)

set(create_command ${RAW2BMX}
    --regtest-real
    -t op1a
    --rdd32
    -o ${output_file}
    --clip test
    --avc_high_422 "${TEST_SOURCE_DIR}/xavc_longgop_50mbit_1080p50.h264"
    -q 16 --locked true --pcm audio_longgop
    -q 16 --locked true --pcm audio_longgop
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
    "longgop.md5"
    ""
    ""
)
