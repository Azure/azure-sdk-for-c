# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_storage_blobs_test LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(AddTestCMocka)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13")
    # Generate map file
    if(MSVC)
        add_link_options(/MAP)
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
        add_link_options(-Wl,-map,az_storage_blobs_test.map)
        add_link_options(-Os)
    else()
        add_link_options(-Xlinker -Map=az_storage_blobs_test.map)
        add_link_options(-Os)
    endif()
endif()


add_cmocka_test(az_storage_blobs_test SOURCES
                main.c
                az_storage_blobs_unit_tests.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                LINK_TARGETS                    
                    az_storage_blobs
                    az_core
                    az_nohttp
                    ${PAL}
                )

