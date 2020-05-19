# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)
set(TARGET_NAME "az_iot_common_test")

project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(AddTestCMocka)

add_cmocka_test(${TARGET_NAME} SOURCES
                main.c
                test_az_iot_common.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                LINK_TARGETS                        
                    az_iot_common
                    az_core
                    ${PAL}                
                )
