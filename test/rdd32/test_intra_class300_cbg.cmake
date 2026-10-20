# Test creating an RDD 32 (XAVC) MXF OP1a file containing 4K Intra "Class 300" CBG
# (Constant Bytes per Element) essence. --avc-cbg must be set explicitly - unlike RP 2027
# AVCI, Class 300/480 CBG essence on the generic AVC path cannot be detected automatically.
# Single Essence Location Style (no body partitions) is expected as a result.
# The essence file has 2 frames, each exactly 1250304 bytes (SMPTE RDD 32 Table C.3 Class 300
# coded frame size), generated from a real 3840x2160 4:2:2 Intra encode padded with an H.264
# filler data NAL unit to reach the constant target size.

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file test_intra_class300_cbg.mxf)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_intra_class300_cbg.mxf)
else()
    set(output_file test_intra_class300_cbg.mxf)
endif()

set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 1
    -d 2
    audio_intra_class300_cbg
)

set(create_command ${RAW2BMX}
    --regtest-real
    -t op1a
    --rdd32
    --avc-cbg
    -o ${output_file}
    --clip test
    --avc_high_422_intra "${TEST_SOURCE_DIR}/intra_class300_cbg.h264"
    -q 16 --locked true --pcm audio_intra_class300_cbg
    -q 16 --locked true --pcm audio_intra_class300_cbg
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
    "intra_class300_cbg.md5"
    ""
    ""
)
