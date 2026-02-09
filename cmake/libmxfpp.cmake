if(MXFpp_link_lib)
    return()
endif()


if(BMX_BUILD_LIBMXFPP_LIB)
    include(FindPkgConfig)

    pkg_search_module(PC_libMXFpp libMXF++ REQUIRED IMPORTED_TARGET)
    message("-- Build using LIBMXF++ pkg-config '${PC_libMXFpp_MODULE_NAME}'")

    set(MXFpp_link_lib PkgConfig::PC_libMXFpp)
else()
    if(NOT EXISTS ${PROJECT_SOURCE_DIR}/deps/libMXFpp/CMakeLists.txt)
        message(FATAL_ERROR
            "libMXF++ source code does not exist at 'deps/libMXFpp'"
        )
    endif()

    include(FetchContent)

    set(LIBMXFPP_SET_MSVC_RUNTIME ${BMX_SET_MSVC_RUNTIME} CACHE INTERNAL "")
    if(BMX_BUILD_LIB_ONLY)
        set(LIBMXFPP_BUILD_LIB_ONLY ON CACHE BOOL "Build MXF++ and MXF libraries only")
    endif()

    FetchContent_Declare(libMXFpp
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/libMXFpp
    )

    FetchContent_MakeAvailable(libMXFpp)

    set(MXFpp_link_lib MXFpp)
 endif()
