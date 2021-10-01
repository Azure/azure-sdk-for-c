# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_storage_blobs_test LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(AddCMockaTest)

add_cmocka_test(
  az_storage_blobs_test
    SOURCES
      _az_test_http_client.c
      _az_test_http_client.h
      main.c
      test_az_storage_blobs.c
    COMPILE_OPTIONS
      ${DEFAULT_C_COMPILE_FLAGS}
      ${NO_CLOBBERED_WARNING}
    LINK_LIBRARIES
      ${CMOCKA_LIBRARIES}
      az_core
      az_storage_blobs
    INCLUDE_DIRECTORIES
      ${CMOCKA_INCLUDE_DIR}
)

create_map_file(az_storage_blobs_test az_storage_blobs_test.map)

add_cmocka_test_environment(az_storage_blobs_test)
