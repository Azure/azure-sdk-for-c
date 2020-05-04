# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

set(TARGET_NAME "az_iot_pnp")

project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(CheckAndIncludeCodeCov)

add_library (
    ${TARGET_NAME}
    src/az_iot_pnp_client.c
    src/az_iot_pnp_client_telemetry.c
)

target_include_directories (${TARGET_NAME} PUBLIC inc)
target_include_directories (${TARGET_NAME} PRIVATE src)
target_include_directories (${TARGET_NAME} PRIVATE 
  ${az_SOURCE_DIR}/sdk/iot/common/inc
  ${az_SOURCE_DIR}/sdk/iot/hub/inc
)

# TODO: Follow up with team about the linking/including strategy for SDK-clients on top of az_core
target_link_libraries(${TARGET_NAME} PRIVATE az_core az_iot_hub)

add_library (az::iot::pnp ALIAS ${TARGET_NAME})

# set coverage excluding for az_core and az_iot_hub. Don't show coverage outside pnp
set(COV_EXCLUDE
    ${az_SOURCE_DIR}/sdk/core/core/inc/*
    ${az_SOURCE_DIR}/sdk/core/core/internal/*
    ${az_SOURCE_DIR}/sdk/iot/hub/inc/*
    ${az_SOURCE_DIR}/sdk/iot/common/inc/*)
include(CreateCodeCoverageTargets)
