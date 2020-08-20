# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_storage_blobs_test LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(AddTestCMocka)

add_cmocka_test(az_storage_blobs_test SOURCES
                main.c
                az_storage_blobs_unit_tests.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                LINK_TARGETS                    
                    az_storage_blobs
                    az_core
                )
