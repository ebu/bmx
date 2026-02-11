if(MSVC)
    # embeds the manifest for long_path support in Windows 10 (1607 and later)
    set(MANIFEST_NAME "BMX Tool")

    configure_file(
        "${PROJECT_SOURCE_DIR}/cmake/bmx_windows.manifest.in"
        "${CMAKE_CURRENT_BINARY_DIR}/bmx_windows.generated.manifest"
        @ONLY
    )

    add_link_options("/MANIFEST:EMBED" "/MANIFESTINPUT:${CMAKE_CURRENT_BINARY_DIR}/bmx_windows.generated.manifest")

    # /manifest file is not found when incremental is enabled (default Debug build config), so we disable it.
    add_link_options("$<$<CONFIG:Debug>:/INCREMENTAL:NO>")

endif()

add_subdirectory(writers)
add_subdirectory(bmxparse)
add_subdirectory(bmxtranswrap)
add_subdirectory(mxf2raw)
add_subdirectory(raw2bmx)


add_executable(bmxtimecode
    bmxtimecode.cpp
)

target_include_directories(bmxtimecode PRIVATE
    "${PROJECT_BINARY_DIR}"
)
target_compile_definitions(bmxtimecode PRIVATE
    HAVE_CONFIG_H
)

target_link_libraries(bmxtimecode PRIVATE
    bmx
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(bmxtimecode "${CMAKE_CURRENT_LIST_DIR}" "bmx")

install(TARGETS bmxtimecode DESTINATION ${CMAKE_INSTALL_BINDIR})
