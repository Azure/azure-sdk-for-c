# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (blobs_client_sample LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_executable (blobs_client_sample src/blobs_client_sample.c)

if(TRANSPORT_CURL)
  set(AZSDK_TRANSPORT_IMPL az::curl)
else()
  set(AZSDK_TRANSPORT_IMPL az::nohttp)
endif()

target_link_libraries(blobs_client_sample PRIVATE az::storage::blobs ${AZSDK_TRANSPORT_IMPL})

if(CMAKE_C_COMPILER_ID MATCHES "GNU")
  set_target_properties(
    blobs_client_sample PROPERTIES
    LINK_FLAGS "-Wl,--start-group"
    LINK_FLAGS $<TARGET_FILE:az::core>
    LINK_FLAGS $<TARGET_FILE:${AZSDK_TRANSPORT_IMPL}>
    LINK_FLAGS "-Wl,--end-group"
  )
endif()

create_map_file(blobs_client_sample blobs_client_sample.map)

