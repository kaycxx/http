option(BUILD_TESTING "Build unit tests" ON)
if(NOT BUILD_TESTING)
    return()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/GitDeps.cmake")

find_package(PkgConfig REQUIRED)
pkg_check_modules(CPP_HTTPLIB REQUIRED IMPORTED_TARGET cpp-httplib>=0.17.0)

git_require(kaycxx::test 0.1.0)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    test/*.cpp
)
list(SORT TEST_SOURCES)

add_executable(${NAME}-tests
    ${TEST_SOURCES}
)
target_include_directories(${NAME}-tests PRIVATE
    test
)
target_link_libraries(${NAME}-tests PRIVATE
    ${NAMESPACE}::${NAME}
    ${NAMESPACE}::test
    PkgConfig::CPP_HTTPLIB
)

set(TEST_DISCOVERY_FILE "${CMAKE_CURRENT_BINARY_DIR}/${NAME}-tests.cmake")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/CTestTestfile.cmake" "include(\"${TEST_DISCOVERY_FILE}\" OPTIONAL)\n")

add_custom_command(TARGET ${NAME}-tests POST_BUILD
    COMMAND ${NAME}-tests --write-ctest "${TEST_DISCOVERY_FILE}"
    COMMENT "Discovering ${PACKAGE_NAME} tests"
    VERBATIM
)

add_custom_target(test
    COMMAND ${NAME}-tests
    DEPENDS ${NAME}-tests
    USES_TERMINAL
)

add_custom_target(ctest
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS ${NAME}-tests
    USES_TERMINAL
)
