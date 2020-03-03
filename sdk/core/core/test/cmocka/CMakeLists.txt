project(core-utest C)

include(AddTestCMocka)

if(DEFINED ENV{AZ_SDK_CODE_COV} AND CMAKE_C_COMPILER_ID MATCHES "GNU")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        include(CodeCoverage)
    endif()
endif()

add_cmocka_test(az_json SOURCES
                test_az_json.c
                test_json_value.c
                test_json_string.c
                test_json_pointer.c
                test_json_parser.c
                test_json_get_by_pointer.c
                test_json_builder.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                # grant access to Private functions to test az_json_private
                PRIVATE_ACCESS ON)

add_cmocka_test(az_span SOURCES
                test_az_span.c
                test_span.c
                test_span_replace.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                # grant access to Private functions to test az_json_private
                PRIVATE_ACCESS ON)

# codeCoverage
if(DEFINED ENV{AZ_SDK_CODE_COV} AND CMAKE_C_COMPILER_ID MATCHES "GNU")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        APPEND_COVERAGE_COMPILER_FLAGS()

        # Basic coverage using lcov (gcc integrated)
        setup_target_for_coverage_lcov(NAME az_json_cov EXECUTABLE az_span)
        
        # HTML and XML - Coverage using gcovr (Needs to be installed into system)
        setup_target_for_coverage_gcovr_html(NAME az_json_cov_html EXECUTABLE az_json)
        setup_target_for_coverage_gcovr_xml(NAME az_json_cov_xml EXECUTABLE az_json)
    endif() 
endif()

add_cmocka_test(az_context SOURCES
                test_az_context.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS})

add_cmocka_test(az_log SOURCES
                test_log.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                # grant access to Private functions to test az_json_private
                PRIVATE_ACCESS ON)

add_cmocka_test(az_http SOURCES
                test_az_http.c
                test_http_request.c
                test_http_response_read_and_parse.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                # grant access to Private functions to test az_json_private
                PRIVATE_ACCESS ON)

add_cmocka_test(az_url_encode SOURCES
                test_url_encode.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                # grant access to Private functions to test az_json_private
                PRIVATE_ACCESS ON)

