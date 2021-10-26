# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (blobs_client_sample LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_executable (blobs_client_sample src/blobs_client_sample.c)

if(TRANSPORT_CURL)
  SET(HTTP_TRANSPORT az::curl)
else()
  SET(HTTP_TRANSPORT az::nohttp)
endif()

target_link_libraries(blobs_client_sample PRIVATE ${HTTP_TRANSPORT} az::core az::storage::blobs)

create_map_file(blobs_client_sample blobs_client_sample.map)
