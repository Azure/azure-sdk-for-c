# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

set(TARGET_NAME "az_keyvault")

project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(CheckAndIncludeCodeCov)

add_library (
  ${TARGET_NAME}
  src/az_keyvault_client.c
  )

target_include_directories (${TARGET_NAME} PUBLIC inc)
target_include_directories (${TARGET_NAME} PRIVATE src)

target_include_directories (${TARGET_NAME} PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/sdk/core/core/inc> $<INSTALL_INTERFACE:include/az_core>)
# include internal headers
target_include_directories(${TARGET_NAME} PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/sdk/core/core/internal> $<INSTALL_INTERFACE:include/az_core_internal>)

# make sure that users can consume the project as a library.
add_library (az::keyvault ALIAS ${TARGET_NAME})

# set coverage excluding for az_core. Don't show coverage outside keyvault
set(COV_EXCLUDE
    ${CMAKE_SOURCE_DIR}/sdk/core/core/inc/*
    ${CMAKE_SOURCE_DIR}/sdk/core/core/internal/*)
include(CreateCodeCoverageTargets)