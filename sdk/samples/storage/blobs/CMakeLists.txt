# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (blobs_client_sample LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# HTTP transport library
if(TRANSPORT_CURL)
  set(AZSDK_TRANSPORT_IMPL az::curl)
else()
  set(AZSDK_TRANSPORT_IMPL az::nohttp)
endif()

# Azure Storage Samples Executables

# Blobs Sample
add_executable (blobs_client_sample
  src/blobs_client_sample.c
)

target_link_libraries(blobs_client_sample
  PRIVATE
    az::storage::blobs
    ${AZSDK_TRANSPORT_IMPL}
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU")
  set_target_properties(
    blobs_client_sample PROPERTIES
    LINK_FLAGS "-Wl,--end-group"
    LINK_FLAGS $<TARGET_FILE:${AZSDK_TRANSPORT_IMPL}>
    LINK_FLAGS $<TARGET_FILE:az::core>
    LINK_FLAGS "-Wl,--start-group"
  )
endif()

create_map_file(blobs_client_sample blobs_client_sample.map)

# Blobs Multi-block Sample
add_executable (blobs_client_sample_multiblock
  src/blobs_client_sample_multiblock.c
)

target_link_libraries(blobs_client_sample_multiblock
  PRIVATE
    az::storage::blobs
    ${AZSDK_TRANSPORT_IMPL}
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU")
  set_target_properties(
    blobs_client_sample_multiblock PROPERTIES
    LINK_FLAGS "-Wl,--end-group"
    LINK_FLAGS $<TARGET_FILE:${AZSDK_TRANSPORT_IMPL}>
    LINK_FLAGS $<TARGET_FILE:az::core>
    LINK_FLAGS "-Wl,--start-group"
  )
endif()

create_map_file(blobs_client_sample_multiblock blobs_client_sample_multiblock.map)

# Blobs Append Sample
add_executable (blobs_client_sample_append
  src/blobs_client_sample_append.c
)

target_link_libraries(blobs_client_sample_append
  PRIVATE
    az::storage::blobs
    ${AZSDK_TRANSPORT_IMPL}
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU")
  set_target_properties(
    blobs_client_sample_append PROPERTIES
    LINK_FLAGS "-Wl,--end-group"
    LINK_FLAGS $<TARGET_FILE:${AZSDK_TRANSPORT_IMPL}>
    LINK_FLAGS $<TARGET_FILE:az::core>
    LINK_FLAGS "-Wl,--start-group"
  )
endif()

create_map_file(blobs_client_sample_append blobs_client_sample_append.map)
