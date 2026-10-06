include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("rdd32")

set(tests
    longgop_hd
    longgop_uhd
    intra_class100
    intra_class300_cbg
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("rdd32" "bmx_rdd32_${test}" "${args}")
endforeach()
