find_package(cmocka CONFIG REQUIRED)

enable_testing()
include(CTest)

if (CMAKE_CROSSCOMPILING)
    if (WIN32)
        find_program(WINE_EXECUTABLE
                     NAMES wine)
        set(TARGET_SYSTEM_EMULATOR ${WINE_EXECUTABLE} CACHE INTERNAL "")
    endif()
endif()

function(ADD_CMOCKA_TEST_ENVIRONMENT _TARGET_NAME)
    if (WIN32 OR CYGWIN OR MINGW)
        file(TO_NATIVE_PATH "${cmocka-library_BINARY_DIR}" CMOCKA_DLL_PATH)

        if (TARGET_SYSTEM_EMULATOR)
            set(DLL_PATH_ENV "WINEPATH=${CMOCKA_DLL_PATH};$ENV{WINEPATH}")
        else()
            set(DLL_PATH_ENV "PATH=${CMOCKA_DLL_PATH};$ENV{PATH}")
        endif()
        #
        # IMPORTANT NOTE: The set_tests_properties(), below, internally
        # stores its name/value pairs with a semicolon delimiter.
        # because of this we must protect the semicolons in the path
        #
        string(REPLACE ";" "\\;" DLL_PATH_ENV "${DLL_PATH_ENV}")

        set_tests_properties(${_TARGET_NAME}
                             PROPERTIES
                                ENVIRONMENT
                                    "${DLL_PATH_ENV}")
    endif()
endfunction()

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

    target_include_directories(${_TARGET_NAME} PRIVATE ${CMOCKA_INCLUDE_DIR})
    
    if (DEFINED _add_cmocka_test_PRIVATE_ACCESS)
        target_include_directories(${_TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/sdk/src/azure/core/)
    endif()

    add_test(${_TARGET_NAME}
        ${TARGET_SYSTEM_EMULATOR} ${_TARGET_NAME}
    )

    add_cmocka_test_environment(${_TARGET_NAME})

endfunction (ADD_CMOCKA_TEST)
