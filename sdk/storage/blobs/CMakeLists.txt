# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

set(TARGET_NAME "az_storage_blobs")
project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(CheckAndIncludeCodeCov)

add_library (
  ${TARGET_NAME}
  src/az_storage_blobs_blob_client.c
  )

target_include_directories (${TARGET_NAME} PUBLIC inc)

target_include_directories (${TARGET_NAME} PUBLIC
  $<BUILD_INTERFACE:${az_SOURCE_DIR}/sdk/core/az_core/inc>
  $<INSTALL_INTERFACE:include/az_core>
)
# include internal headers
target_include_directories(${TARGET_NAME} PUBLIC
  $<BUILD_INTERFACE:${az_SOURCE_DIR}/sdk/core/az_core/internal>
  $<INSTALL_INTERFACE:include/az_core_internal>
)
# make sure that users can consume the project as a library.
add_library (az::storage::blobs ALIAS ${TARGET_NAME})

# set coverage excluding for az_core. Don't show coverage outside storage blobs
set(COV_EXCLUDE
    ${az_SOURCE_DIR}/sdk/core/az_core/inc/*
    ${az_SOURCE_DIR}/sdk/core/az-core/internal/*)
include(CreateCodeCoverageTargets)
