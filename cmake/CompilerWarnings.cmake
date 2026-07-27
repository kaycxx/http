if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(
        # cspell: disable
        -Werror
        -Wall
        -Wextra
        -Wpedantic
        # Additional useful diagnostics not covered by the base trio above
        # on GCC 14, or stricter levels than the defaults they enable.
        -Walloca
        -Warith-conversion
        -Wcast-align=strict
        -Wcast-qual
        -Wconversion
        -Wdate-time
        -Wdouble-promotion
        -Wduplicated-branches
        -Wduplicated-cond
        -Wfloat-equal
        -Wformat=2
        -Wformat-overflow=2
        -Wformat-signedness
        -Wformat-truncation=2
        -Wimplicit-fallthrough=5
        -Wlogical-op
        -Wmissing-declarations
        -Wmissing-include-dirs
        -Wnon-virtual-dtor
        -Wnull-dereference
        -Wold-style-cast
        -Woverloaded-virtual
        -Wplacement-new=2
        -Wredundant-decls
        -Wsign-conversion
        -Wstrict-null-sentinel
        -Wsuggest-override
        -Wswitch-enum
        -Wundef
        -Wuseless-cast
        -Wvolatile
        -Wzero-as-null-pointer-constant
        # cspell: enable
    )
endif()
