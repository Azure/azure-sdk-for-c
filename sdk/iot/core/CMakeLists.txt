# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

set(TARGET_NAME "az_iot")

project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(CheckAndIncludeCodeCov)

add_library (
    ${TARGET_NAME}
    src/az_iot_sas_token.c
    src/az_iot_telemetry.c
)

target_include_directories (${TARGET_NAME} PUBLIC inc)
target_include_directories (${TARGET_NAME} PRIVATE src)

# TODO: Follow up with team about the linking/including strategy for SDK-clients on top of az_core
target_link_libraries(${TARGET_NAME} PRIVATE az_core)

add_library (az::iot ALIAS ${TARGET_NAME})

# set coverage excluding for az_core. Don't show coverage outside iot
set(COV_EXCLUDE
    ${CMAKE_SOURCE_DIR}/sdk/core/core/inc/*
    ${CMAKE_SOURCE_DIR}/sdk/core/core/internal/*)
include(CreateCodeCoverageTargets)
