# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (blobs_client_sample LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library(core src/core.c)

add_library(http STATIC src/http.c)
target_link_libraries(http PUBLIC core)

add_library(stg src/stg.c)
target_link_libraries(stg PUBLIC core)

add_executable(exe src/exe.c)
set_target_properties(core http stg exe PROPERTIES LINK_INTERFACE_MULTIPLICITY 30)
target_link_libraries(exe PRIVATE http stg core)

# add_executable (blobs_client_sample src/blobs_client_sample.c)
# 
# target_link_libraries(blobs_client_sample PRIVATE az::storage::blobs)
# 
# if(TRANSPORT_CURL)
#   target_link_libraries(blobs_client_sample PRIVATE az::curl)
# else()
#   target_link_libraries(blobs_client_sample PRIVATE az::nohttp)
# endif()
# 
# create_map_file(blobs_client_sample blobs_client_sample.map)
# 
