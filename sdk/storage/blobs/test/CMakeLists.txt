# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_core_test LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_executable (az_storage_blobs_test main.c)
target_link_libraries(az_storage_blobs_test PRIVATE az_storage_blobs az_core)

add_test(NAME az_storage_blobs_test COMMAND az_storage_blobs_test)
