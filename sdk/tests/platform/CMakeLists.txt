# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_platform_test LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(AddCMockaTest)

add_cmocka_test(az_platform_test SOURCES
                main.c
                test_az_platform.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS} ${NO_CLOBBERED_WARNING}
                LINK_LIBRARIES ${CMOCKA_LIB} az_core
                # include cmocka headers and private folder headers
                INCLUDE_DIRECTORIES ${CMOCKA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/sdk/src/azure/core/
                )

create_map_file(az_platform_test az_platform_test.map)

add_cmocka_test_environment(az_platform_test)
