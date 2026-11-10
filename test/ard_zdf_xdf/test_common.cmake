include("${TEST_SOURCE_DIR}/../testing.cmake")


function(run_test test_name video_opt_name video_file duration)
    if(TEST_MODE STREQUAL "check")
        set(output_file test_${test_name}.mxf)
        set(output_info_file test_${test_name}.xml)
    elseif(TEST_MODE STREQUAL "samples")
        file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

        set(output_file ${BMX_TEST_SAMPLES_DIR}/test_${test_name}.mxf)
        set(output_info_file ${BMX_TEST_SAMPLES_DIR}/test_${test_name}.xml)
    else()
        set(output_file test_${test_name}.mxf)
        set(output_info_file ${TEST_SOURCE_DIR}/${test_name}.xml)
    endif()

    set(create_test_audio ${CREATE_TEST_ESSENCE}
        -t 1
        -d ${duration}
        audio_${test_name}
    )

    # ARD_ZDF_XDF-01: XAVC Long GOP 50 Mbit/s, 1080p50, 8/16 mono AES3 PCM tracks
    # (24 bit, 48 kHz, locked)
    set(create_command ${RAW2BMX}
        --regtest-real
        -t op1a
        --ard-zdf-xdf
        -f 50
        -y 10:00:00:00
        -o ${output_file}
        --afd 10
        --${video_opt_name} "${TEST_SOURCE_DIR}/${video_file}"
        -q 24 --locked true --pcm audio_${test_name}
        -q 24 --locked true --pcm audio_${test_name}
        -q 24 --locked true --pcm audio_${test_name}
        -q 24 --locked true --pcm audio_${test_name}
        -q 24 --locked true --pcm audio_${test_name}
        -q 24 --locked true --pcm audio_${test_name}
        -q 24 --locked true --pcm audio_${test_name}
        -q 24 --locked true --pcm audio_${test_name}
    )

    run_test_a(
        "${TEST_MODE}"
        "${BMX_TEST_WITH_VALGRIND}"
        "${create_test_audio}"
        ""
        ""
        "${create_command}"
        ""
        ""
        ""
        "${output_file}"
        "${test_name}.md5"
        ""
        ""
    )
endfunction()
