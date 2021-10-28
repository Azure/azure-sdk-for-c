# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (blobs_client_sample LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_executable(blobs_client_sample src/blobs_client_sample.c)

if(TRANSPORT_CURL)
  set(AZURE_SDK az::curl az::storage::blobs)
else()
  set(AZURE_SDK az::nohttp az::storage::blobs)
endif()

if (CMAKE_C_COMPILER_ID MATCHES "GNU")
  add_library(gcc_fix src/gcc_fix.c)
  target_link_libraries(gcc_fix PUBLIC ${AZURE_SDK})
  set(AZURE_SDK gcc_fix)
endif()

target_link_libraries(blobs_client_sample PRIVATE ${AZURE_SDK})

create_map_file(blobs_client_sample blobs_client_sample.map)
