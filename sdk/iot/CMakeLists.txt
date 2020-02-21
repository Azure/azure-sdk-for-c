# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_iot LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library (
    az_iot
    src/az_iot_sas_token.c
)

target_include_directories (az_iot PUBLIC inc)
target_include_directories (az_iot PRIVATE src)

target_link_libraries(az_iot PUBLIC az_core)
target_link_libraries(az_iot PRIVATE az_core_internal)

add_library (az::iot ALIAS az_iot)

add_subdirectory(tests)
