# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

set(TARGET_NAME "az_iot_common")

project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(CheckAndIncludeCodeCov)

add_library (
    ${TARGET_NAME}
    src/az_iot_common.c
)

target_include_directories (${TARGET_NAME} PUBLIC inc)

target_link_libraries(${TARGET_NAME} PUBLIC
  az_core
)

add_library (az::iot::common ALIAS ${TARGET_NAME})

# set coverage excluding for az_core. Don't show coverage outside iot
set(COV_EXCLUDE
    ${az_SOURCE_DIR}/sdk/core/core/inc/*
    ${az_SOURCE_DIR}/sdk/core/core/internal/*)
include(CreateCodeCoverageTargets)
