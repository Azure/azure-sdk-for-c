# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)
set(TARGET_NAME "az_core_test")

project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(AddTestCMocka)

# -ld link option is only available for gcc
if(UNIT_TESTING_MOCK_ENABLED)
    set(WRAP_FUNCTIONS "-Wl,--wrap=az_platform_clock_msec -Wl,--wrap=az_http_client_send_request")
else()
    set(WRAP_FUNCTIONS "")
endif()

add_cmocka_test(${TARGET_NAME} SOURCES
                main.c
                test_az_json.c
                test_json_value.c
                test_json_string.c
                test_json_pointer.c
                test_json_parser.c
                test_json_get_by_pointer.c
                test_json_builder.c
                test_az_span.c
                test_span.c
                test_span_replace.c
                test_az_context.c
                test_log.c
                test_http_request.c
                test_http_response_read_and_parse.c
                test_az_pipeline.c
                test_url_encode.c
                test_az_aad.c
                test_az_http_policy.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                LINK_OPTIONS ${WRAP_FUNCTIONS}
                # grant access to Private functions to test az_json_private
                PRIVATE_ACCESS ON)
