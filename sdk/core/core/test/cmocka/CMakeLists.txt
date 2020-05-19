# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)
set(TARGET_NAME "az_core_test")

project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(AddTestCMocka)

# -ld link option is only available for gcc
if(UNIT_TESTING_MOCKS)
    set(WRAP_FUNCTIONS "-Wl,--wrap=az_platform_clock_msec -Wl,--wrap=az_http_client_send_request")
else()
    set(WRAP_FUNCTIONS "")
endif()

add_cmocka_test(${TARGET_NAME} SOURCES
                main.c
                test_az_context.c
                test_az_credential_client_secret.c
                test_az_http.c
                test_az_json.c
                test_az_logging.c
                test_az_pipeline.c
                test_az_policy.c
                test_az_span.c
                test_az_url_encode.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                LINK_OPTIONS ${WRAP_FUNCTIONS}
                # grant access to Private functions to test az_json_private
                PRIVATE_ACCESS ON
                LINK_TARGETS az_core ${PAL} az_nohttp)
