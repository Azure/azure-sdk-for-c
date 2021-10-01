# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (blobs_client_sample LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# make keys client example
add_executable (blobs_client_sample src/blobs_client_sample.c)

create_map_file(blobs_client_sample blobs_client_sample.map)

target_link_libraries(blobs_client_sample PRIVATE az_storage_blobs az_core)

if(TRANSPORT_CURL)
  target_link_libraries(blobs_client_sample PRIVATE az_curl)
else()
  target_link_libraries(blobs_client_sample PRIVATE az_nohttp)
endif()
