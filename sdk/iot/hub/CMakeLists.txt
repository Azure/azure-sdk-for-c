# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

set(TARGET_NAME "az_iot_hub_client")

project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(CheckAndIncludeCodeCov)

add_library (
    ${TARGET_NAME}
    src/az_iot_hub_client.c
    src/az_iot_hub_client_sas.c
    src/az_iot_hub_client_telemetry.c
)

target_include_directories (${TARGET_NAME} PUBLIC inc)
target_include_directories (${TARGET_NAME} PRIVATE src)

target_link_libraries(${TARGET_NAME} PRIVATE az_core)
target_link_libraries(${TARGET_NAME} PRIVATE az_iot_core)

add_library (az::iot ALIAS ${TARGET_NAME})

# set coverage excluding for az_core. Don't show coverage outside iot
set(COV_EXCLUDE
    ${CMAKE_SOURCE_DIR}/sdk/core/core/inc/*
    ${CMAKE_SOURCE_DIR}/sdk/core/core/internal/*)
include(CreateCodeCoverageTargets)
