# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)
set(TARGET_NAME "az_core_test")

project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(AddTestCMocka)

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

                test_url_encode.c

                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                # grant access to Private functions to test az_json_private
                PRIVATE_ACCESS ON)
