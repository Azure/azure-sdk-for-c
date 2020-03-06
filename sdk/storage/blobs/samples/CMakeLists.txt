# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_storage_blobs_samples LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# make keys client example
add_executable (blobs_client_example src/blobs_client_example.c)
target_link_libraries(blobs_client_example PRIVATE az_storage_blobs az_core)

if (UNIX)
  target_link_libraries(blobs_client_example PRIVATE m)
endif()

# TODO, Add tests for samples
# add_test(NAME az_storage_blobs_samples_test COMMAND az_storage_blobs_samples_test)
