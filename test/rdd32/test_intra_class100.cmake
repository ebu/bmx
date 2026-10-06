# Test creating an RDD 32 (XAVC) MXF OP1a file containing RP 2027 AVCI100 (Intra "Class 100")
# essence. Single Essence Location Style (no body partitions) is expected, selected
# automatically - --avc-cbg is NOT required/used for RP 2027 AVCI essence.

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file test_intra_class100.mxf)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_intra_class100.mxf)
else()
    set(output_file test_intra_class100.mxf)
endif()

set(create_test_video ${CREATE_TEST_ESSENCE}
    -t 8
    -d 3
    video_intra_class100
)

set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 1
    -d 3
    audio_intra_class100
)

set(create_command ${RAW2BMX}
    --regtest
    -t op1a
    --rdd32
    -o ${output_file}
    --clip test
    --avci100_1080p video_intra_class100
    -q 16 --locked true --pcm audio_intra_class100
    -q 16 --locked true --pcm audio_intra_class100
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_video}"
    "${create_test_audio}"
    ""
    "${create_command}"
    ""
    ""
    ""
    "${output_file}"
    "intra_class100.md5"
    ""
    ""
)
