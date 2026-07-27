# Copyright (c) 2026 Klaus Reimer
# SPDX-License-Identifier: MIT
# Version: 2.0.0
# Source: https://github.com/kaycxx/cmake-git-deps

if(COMMAND git_require)
    return()
endif()

include(FetchContent)

function(git_require target version)
    if(TARGET "${target}")
        return()
    endif()

    cmake_parse_arguments(PARSE_ARGV 2 ARG "" "PACKAGE;URL" "")

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid git_require() arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()

    if("${target}" MATCHES "^([^:]+)::([^:]+)$")
        set(owner "${CMAKE_MATCH_1}")
        set(repo "${CMAKE_MATCH_2}")
    elseif(NOT DEFINED ARG_PACKAGE OR NOT DEFINED ARG_URL)
        message(FATAL_ERROR
            "Target '${target}' does not match '<owner>::<repo>'. "
            "Pass PACKAGE and URL explicitly."
        )
    endif()

    if(DEFINED ARG_PACKAGE)
        set(package "${ARG_PACKAGE}")
    else()
        set(package "${owner}-${repo}")
    endif()

    if(DEFINED ARG_URL)
        set(git_url "${ARG_URL}")
    else()
        set(git_url "https://github.com/${owner}/${repo}.git")
    endif()

    FetchContent_Declare(
        "${package}"
        GIT_REPOSITORY "${git_url}"
        GIT_TAG "refs/tags/v${version}"
        EXCLUDE_FROM_ALL
        FIND_PACKAGE_ARGS "${version}" CONFIG
    )

    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable("${package}")
endfunction()
