include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("timed_events")

foreach(test_num RANGE 1 11)
    set(args
        "${common_args}"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test_${test_num}.cmake
    )
    setup_test("timed_events" "bmx_timed_events_${test_num}" "${args}")
endforeach()
