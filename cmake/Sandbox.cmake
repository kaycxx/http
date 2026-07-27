file(GLOB SANDBOX_SOURCES CONFIGURE_DEPENDS
    sandbox/*.cpp
)
list(SORT SANDBOX_SOURCES)

foreach(SANDBOX_SOURCE IN LISTS SANDBOX_SOURCES)
    get_filename_component(SANDBOX_NAME "${SANDBOX_SOURCE}" NAME_WE)

    add_executable(sandbox-${SANDBOX_NAME}
        EXCLUDE_FROM_ALL
        "${SANDBOX_SOURCE}"
    )
    target_link_libraries(sandbox-${SANDBOX_NAME} PRIVATE
        ${NAMESPACE}::${NAME}
    )
endforeach()
