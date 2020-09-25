# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT
#
# Check that cmocka is installed and available to be used.
# Also creates functions to create exe tests with cmocka
#

## First check cmocka is available, will stop generation if not available
find_package(cmocka CONFIG REQUIRED)

# Ask cmake to enable testing to have ctest availabe
enable_testing()
include(CTest)

###
#
# Function to create tests with cmocka
# Parameters:
# name: Required parameter. A cmake target will be created base on the name of this target
#              for example, if name is `a`, this function will create an a runnable file.
#
# src: source files to build
#
# linker_args: Any option to be set for linking runnable file. For example, wrapping options like -Wl
# private_access: Use ON to give acccess to private functions to the tests.
# link_targets: Any cmake target to be linked to the tests. For example, adding libs from the same cmake project
function(CREATE_CMOCKA_TEST _NAME)
    
    # define args
    set(args_
        SRC
        LINKER_ARGS
        PRIVATE_ACCESS
        LINK_TARGETS
    )
    # parse args and make them accessible through `_args`
    cmake_parse_arguments(_args
        ""
        ""
        "${args_}"
        ${ARGN}
    )

    # Creates runnable
    add_executable(${_NAME} ${_args_SRC})

    # when using VCPKG, use the LIB Variable
    if(DEFINED ENV{VCPKG_ROOT} OR DEFINED ENV{VCPKG_INSTALLATION_ROOT})
        set(CMOCKA_LIB ${CMOCKA_LIBRARIES})
    else()
        # consume from system installed lib
        set(CMOCKA_LIB cmocka)
    endif()

    # add math lib on Linux only
    set(MATH_LIB_UNIX "")
    if (UNIX)
        set(MATH_LIB_UNIX "m")
    endif()

    if (DEFINED _args_LINK_TARGETS)
        # link to user defined
        target_link_libraries(${_NAME}
            PRIVATE ${CMOCKA_LIB} ${_args_LINK_TARGETS} ${MATH_LIB_UNIX}
        )   
    else()
        # link against az_core by default
        target_link_libraries(${_NAME}
            PRIVATE ${CMOCKA_LIB} az_core ${MATH_LIB_UNIX}
        )
    endif()

    if (DEFINED _args_LINKER_ARGS)
        set_target_properties(${_NAME}
            PROPERTIES LINK_FLAGS
            ${_args_LINKER_ARGS}
        )
    endif()

    # Workaround for linker warning LNK4098: defaultlib 'LIBCMTD' conflicts with use of other libs
    if (MSVC)
     set_target_properties(${_NAME}
            PROPERTIES
            LINK_FLAGS
            "/NODEFAULTLIB:libcmtd.lib"
            LINK_FLAGS_RELEASE
            "/NODEFAULTLIB:libcmt.lib"
        )
    endif()

    # headers for cmocka
    target_include_directories(${_NAME} PRIVATE ${CMOCKA_INCLUDE_DIR})
    
    # grant access to az-core private headers if requested
    if (DEFINED _args_PRIVATE_ACCESS)
        target_include_directories(${_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/sdk/src/azure/core/)
    endif()

    # add tests for ctest
    add_test(${_NAME}
        ${TARGET_SYSTEM_EMULATOR} ${_NAME}
    )

endfunction (CREATE_CMOCKA_TEST)
