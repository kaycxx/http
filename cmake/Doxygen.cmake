configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/Doxyfile.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
    @ONLY
)

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    add_custom_target(apidoc
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${CMAKE_CURRENT_BINARY_DIR}/apidoc"
        COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating API documentation"
        VERBATIM
    )
else()
    add_custom_target(apidoc
        COMMAND "${CMAKE_COMMAND}" -E echo "Doxygen not found. Install doxygen to generate API documentation."
        COMMAND "${CMAKE_COMMAND}" -E false
        COMMENT "Doxygen not found"
        VERBATIM
    )
endif()
