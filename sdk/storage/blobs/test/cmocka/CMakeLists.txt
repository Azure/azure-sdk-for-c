# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)
set(TARGET_NAME "az_storage_blobs_test")

project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(AddTestCMocka)

add_cmocka_test(${TARGET_NAME} SOURCES
                main.c
                az_storage_blobs_unit_tests.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                LINK_TARGETS                    
                    az_storage_blobs
                    az_core
                    az_nohttp
                    ${PAL}
                )
