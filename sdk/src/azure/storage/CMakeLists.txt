# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_storage_blobs LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(CheckAndIncludeCodeCov)

add_library (
  az_storage_blobs
  ${CMAKE_CURRENT_LIST_DIR}/az_storage_blobs_blob_client.c
  )

target_include_directories (az_storage_blobs PUBLIC inc)

target_link_libraries(az_storage_blobs
  PUBLIC
    az::core
)

# make sure that users can consume the project as a library.
add_library (az::storage::blobs ALIAS az_storage_blobs)

# set coverage excluding for az_core. Don't show coverage outside storage blobs
set(COV_EXCLUDE
    ${az_SOURCE_DIR}/sdk/inc/azure/core/*
    ${az_SOURCE_DIR}/sdk/inc/azure/core/internal/*)

create_code_coverage_targets(az_storage_blobs)
