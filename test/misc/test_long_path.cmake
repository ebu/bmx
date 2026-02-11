# Test if long path support is working (>260 chars)
# We assume if it works for RAW2BMX it will work for other tools as well
# 400 char length is > MAX_PATH (260) and < HFS+ 1024 and EXT 4096 char limit

set(test_name long_path)

set(LONGPATH_DIR_ROOT "${BMX_TEST_SAMPLES_DIR}/long_path_test")
set(LONGPATH_DIR ${LONGPATH_DIR_ROOT})

#create 400 char path
foreach(i RANGE 1 20)
    string(APPEND LONGPATH_DIR "/lvl_${i}_long_path_dir")
endforeach()

# Create the directory physically on the disk
file(MAKE_DIRECTORY "${LONGPATH_DIR}")

if(TEST_MODE STREQUAL "samples")
    set(output_file "${LONGPATH_DIR}/test_${test_name}.mxf")
else()
    set(output_file "${LONGPATH_DIR}/output.mxf")
endif()

include("${TEST_SOURCE_DIR}/test_common.cmake")

set(create_command ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -o "${output_file}"
    --avc_high_422_intra video_${test_name}
    -q 24 --locked true --pcm audio_${test_name}_1
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_audio_1}"
    ""
    "${create_test_video}"
    "${create_command}"
    ""
    ""
    ""
    "${output_file}"
    "${test_name}.md5"
    ""
    ""
)
