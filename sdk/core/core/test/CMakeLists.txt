# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_core_test LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_executable (
    az_core_test 
    main.c 
    test_http_request.c
    test_http_response_read_and_parse.c    
    test_json_builder.c
    test_json_get_by_pointer.c
    test_json_parser.c
    test_json_pointer.c
    test_json_string.c
    test_json_value.c
    test_log.c
    test_span.c
    test_span_builder_replace.c
    test_url_encode.c
    )

 target_link_libraries(az_core_test PRIVATE az_core)
 target_include_directories(az_core_test PRIVATE ../src)
 target_include_directories(az_core_test PRIVATE .)

 add_test(NAME az_core_test COMMAND az_core_test)
