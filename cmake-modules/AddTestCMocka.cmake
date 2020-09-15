#
# Copyright (c) 2007      Daniel Gollub <dgollub@suse.de>
# Copyright (c) 2007-2018 Andreas Schneider <asn@cryptomilk.org>
# Copyright (c) 2018      Anderson Toshiyuki Sasaki <ansasaki@redhat.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
#
# Modifed version from https://github.com/xbmc/libssh/blob/667fb5f9a9c96f210583dbfb11755c43250c5e55/cmake/Modules/AddCMockaTest.cmake
#.rst:
# AddTestCMocka
# -------------
#
# This file provides a function to add a test
#
# Functions provided
# ------------------
#
# ::
#
#   add_cmocka_test(target_name
#                   SOURCES src1 src2 ... srcN
#                   [COMPILE_OPTIONS opt1 opt2 ... optN]
#                   [LINK_OPTIONS lopt1 lop2 .. loptN]
#                   [PRIVATE_ACCESS ON/OFF]
#                   [LINK_TARGETS target1 target2 .. targetN]
#                  )
#
# ``target_name``:
#   Required, expects the name of the test which will be used to define a target
#
# ``SOURCES``:
#   Required, expects one or more source files names
#
# ``COMPILE_OPTIONS``:
#   Optional, expects one or more options to be passed to the compiler
#
# ``LINK_OPTIONS``:
#   Optional, expects one or more options to be passed to the linker
#
# ``PRIVATE_ACCESS``:
#   Optional, when ON, tests are granted access to az_core private layer
#
# ``LINK_TARGETS``:
#   Optional, expects one or more targets from the same project to be passed to cmake target linker
#
#
# Example:
#
# .. code-block:: cmake
#
#   add_cmocka_test(my_test
#                   SOURCES my_test.c other_source.c
#                   COMPILE_OPTIONS -g -Wall
#                   LINK_OPTIONS -Wl,--enable-syscall-fixup
#                   PRIVATE_ACCESS ON
#                   LINK_TARGETS target1, target2
#                  )
#
# Where ``my_test`` is the name of the test, ``my_test.c`` and
# ``other_source.c`` are sources for the binary, ``-g -Wall`` are compiler
# options to be used, ``-Wl,--enable-syscall-fixup`` is an option passed to the linker, 
# ``PRIVATE_ACCESS`` is ON to let tests access private layer from az_core and ``LINK_TAGETS```
# list all the cmake tagets to link to
#

find_package(cmocka CONFIG REQUIRED)

enable_testing()
include(CTest)

set(MATH_LIB_UNIX "")
if (UNIX)
    set(MATH_LIB_UNIX "m")
endif()

function(ADD_CMOCKA_TEST _TARGET_NAME)

    set(one_value_arguments
    )

    set(multi_value_arguments
        SOURCES
        COMPILE_OPTIONS
        LINK_OPTIONS
        PRIVATE_ACCESS
        LINK_TARGETS
    )

    cmake_parse_arguments(_add_cmocka_test
        ""
        "${one_value_arguments}"
        "${multi_value_arguments}"
        ${ARGN}
    )

    if (NOT DEFINED _add_cmocka_test_SOURCES)
        message(FATAL_ERROR "No sources provided for target ${_TARGET_NAME}")
    endif()

    add_executable(${_TARGET_NAME} ${_add_cmocka_test_SOURCES})

    # Suppress clobber warning for longjmp
    if(CMAKE_C_COMPILER_ID MATCHES "GNU")
      target_compile_options(${_TARGET_NAME} PRIVATE -Wno-clobbered)
    endif()

    if (DEFINED _add_cmocka_test_COMPILE_OPTIONS)
        target_compile_options(${_TARGET_NAME}
            PRIVATE ${_add_cmocka_test_COMPILE_OPTIONS}
        )
    endif()

    if(DEFINED ENV{VCPKG_ROOT} OR DEFINED ENV{VCPKG_INSTALLATION_ROOT})
        set(CMOCKA_LIB ${CMOCKA_LIBRARIES})
    else()
        set(CMOCKA_LIB cmocka)
    endif()

    if (DEFINED _add_cmocka_test_LINK_TARGETS)
        # link to user defined
        target_link_libraries(${_TARGET_NAME}
            PRIVATE ${CMOCKA_LIB} ${_add_cmocka_test_LINK_TARGETS} ${MATH_LIB_UNIX}
        )   
    else()
        # link against az_core by default
        target_link_libraries(${_TARGET_NAME}
            PRIVATE ${CMOCKA_LIB} az_core ${MATH_LIB_UNIX}
        )
    endif()

    if (DEFINED _add_cmocka_test_LINK_OPTIONS)
        set_target_properties(${_TARGET_NAME}
            PROPERTIES LINK_FLAGS
            ${_add_cmocka_test_LINK_OPTIONS}
        )
    endif()

    # Workaround for linker warning LNK4098: defaultlib 'LIBCMTD' conflicts with use of other libs
    if (MSVC)
     set_target_properties(${_TARGET_NAME}
            PROPERTIES
            LINK_FLAGS
            "/NODEFAULTLIB:libcmtd.lib"
            LINK_FLAGS_RELEASE
            "/NODEFAULTLIB:libcmt.lib"
        )
    endif()

    target_include_directories(${_TARGET_NAME} PRIVATE ${CMOCKA_INCLUDE_DIR})
    
    if (DEFINED _add_cmocka_test_PRIVATE_ACCESS)
        target_include_directories(${_TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/sdk/src/azure/core/)
    endif()

    add_test(${_TARGET_NAME}
        ${TARGET_SYSTEM_EMULATOR} ${_TARGET_NAME}
    )

endfunction (ADD_CMOCKA_TEST)
