include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("hevc")

# Mux+demux test for HEVC video essence.
set(tests
    1
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("hevc" "bmx_hevc_${test}" "${args}")
endforeach()
