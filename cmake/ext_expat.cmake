if(expat_link_lib)
    return()
endif()


if(BMX_BUILD_EXPAT_SOURCE)
    include(FetchContent)

    set(EXPAT_BUILD_DOCS OFF CACHE INTERNAL "")
    set(EXPAT_BUILD_EXAMPLES OFF CACHE INTERNAL "")
    set(EXPAT_BUILD_TESTS OFF CACHE INTERNAL "")
    set(EXPAT_BUILD_TOOLS OFF CACHE INTERNAL "")
    if(BUILD_SHARED_LIBS)
        set(EXPAT_SHARED_LIBS ON CACHE INTERNAL "")
    else()
        set(EXPAT_SHARED_LIBS OFF CACHE INTERNAL "")
    endif()

    if(EXISTS "${PROJECT_SOURCE_DIR}/deps/libexpat")
        FetchContent_Declare(FT_libexpat
            SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/libexpat"
			SOURCE_SUBDIR expat
        )
    else()
        FetchContent_Declare(FT_libexpat
			SOURCE_SUBDIR expat
            GIT_REPOSITORY https://github.com/libexpat/libexpat
            GIT_TAG R_2_8_0
        )
    endif()

    FetchContent_MakeAvailable(FT_libexpat) 

    set(expat_link_lib expat)
else()
    find_package(EXPAT REQUIRED)
    set(expat_link_lib EXPAT::EXPAT)
endif()
