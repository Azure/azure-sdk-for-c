message(STATUS "Looking for CMocka which is required for unit testing")
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

    if (DEFINED _add_cmocka_test_COMPILE_OPTIONS)
        target_compile_options(${_TARGET_NAME}
            PRIVATE ${_add_cmocka_test_COMPILE_OPTIONS}
        )
    endif()

    if (DEFINED _add_cmocka_test_LINK_TARGETS)
        # link to user defined
        target_link_libraries(${_TARGET_NAME}
            PRIVATE ${CMOCKA_LIBRARIES} ${_add_cmocka_test_LINK_TARGETS} ${MATH_LIB_UNIX}
        )
    else()
        # link against az_core by default
        target_link_libraries(${_TARGET_NAME}
            PRIVATE ${CMOCKA_LIBRARIES} az_core ${MATH_LIB_UNIX}
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
        target_include_directories(${_TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/sdk/core/core/src)
    endif()

    add_test(${_TARGET_NAME}
        ${TARGET_SYSTEM_EMULATOR} ${_TARGET_NAME}
    )

    add_cmocka_test_environment(${_TARGET_NAME})

    # codeCoverage
    if(DEFINED ENV{AZ_SDK_CODE_COV} AND CMAKE_C_COMPILER_ID MATCHES "GNU")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            APPEND_COVERAGE_COMPILER_FLAGS()
    
            # Basic coverage using lcov (gcc integrated)
            setup_target_for_coverage_lcov(NAME ${_TARGET_NAME}_cov EXECUTABLE ${_TARGET_NAME})
            
            # HTML and XML - Coverage using gcovr (Needs to be installed into system)
            setup_target_for_coverage_gcovr_html(NAME ${_TARGET_NAME}_cov_html EXECUTABLE ${_TARGET_NAME})
            setup_target_for_coverage_gcovr_xml(NAME ${_TARGET_NAME}_cov_xml EXECUTABLE ${_TARGET_NAME})
        endif() 
    endif()

endfunction (ADD_CMOCKA_TEST)
