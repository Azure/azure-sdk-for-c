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

# Get only headers from core. Do not link. Customers is expected to link
# with az_iot and az_core. This allows sharing az_core with more clients
target_include_directories (az_iot PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/sdk/core/core/inc> $<INSTALL_INTERFACE:include/az_core>)
# include internal headers
target_include_directories(az_iot PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/sdk/core/core/internal> $<INSTALL_INTERFACE:include/az_core_internal>)

add_library (az::iot ALIAS az_iot)

