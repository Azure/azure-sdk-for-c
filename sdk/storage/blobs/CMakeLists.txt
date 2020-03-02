# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_storage_blobs LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library (
  az_storage_blobs
  src/az_storage_blobs_blob_client.c
  )

target_include_directories (az_storage_blobs PUBLIC inc)

target_include_directories (az_storage_blobs PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/sdk/core/core/inc> $<INSTALL_INTERFACE:include/az_core>)
# include internal headers
target_include_directories(az_storage_blobs PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/sdk/core/core/internal> $<INSTALL_INTERFACE:include/az_core_internal>)

# make sure that users can consume the project as a library.
add_library (az::storage::blobs ALIAS az_storage_blobs)
