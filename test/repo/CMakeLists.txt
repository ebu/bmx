include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("repo")

# Combined mux+demux test for the Dolby-added essence types (HEVC + ISXD +
# BW64/S-ADM + IAB) in a single op1a MXF file, following new_tests/run.sh.
set(tests
    1
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("repo" "bmx_repo_${test}" "${args}")
endforeach()
