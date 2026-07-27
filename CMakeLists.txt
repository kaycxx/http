cmake_minimum_required(VERSION 3.31)

set(NAME http)
set(NAMESPACE kaycxx)
set(PACKAGE_NAME ${NAMESPACE}-${NAME})

project(${PACKAGE_NAME}
    VERSION 0.1.0
    DESCRIPTION "HTTP client library"
    HOMEPAGE_URL "https://github.com/${NAMESPACE}/${NAME}"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(cmake/CompilerWarnings.cmake)
include(cmake/GitDeps.cmake)

find_package(CURL 8.13.0 REQUIRED)
find_package(nlohmann_json 3.11.3 CONFIG REQUIRED)
git_require(kaycxx::core 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)
list(SORT SOURCES)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    include/*.hpp
)
list(SORT HEADERS)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

add_library(${PACKAGE_NAME}
    ${SOURCES}
)
add_library(${NAMESPACE}::${NAME} ALIAS ${PACKAGE_NAME})

target_sources(${PACKAGE_NAME} PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES ${HEADERS}
)

set_target_properties(${PACKAGE_NAME} PROPERTIES
    OUTPUT_NAME ${PACKAGE_NAME}
    EXPORT_NAME ${NAME}
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
)

target_include_directories(${PACKAGE_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(${PACKAGE_NAME} PUBLIC cxx_std_${CMAKE_CXX_STANDARD})
target_link_libraries(${PACKAGE_NAME}
    PUBLIC nlohmann_json::nlohmann_json
    PRIVATE CURL::libcurl kaycxx::core
)

include(cmake/Install.cmake)

if(PROJECT_IS_TOP_LEVEL)
    include(cmake/Doxygen.cmake)
    include(cmake/Sandbox.cmake)
    include(cmake/Test.cmake)
endif()
