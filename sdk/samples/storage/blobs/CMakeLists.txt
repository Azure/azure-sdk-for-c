# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (blobs_client_sample LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library(core STATIC src/core.c)
set_target_properties(core PROPERTIES LINK_INTERFACE_MULTIPLICITY 3)

add_library(http STATIC src/http.c)
target_link_libraries(http PUBLIC core)
set_target_properties(http PROPERTIES LINK_INTERFACE_MULTIPLICITY 3)

add_library(stg STATIC src/stg.c)
target_link_libraries(stg PUBLIC core)
set_target_properties(stg PROPERTIES LINK_INTERFACE_MULTIPLICITY 3)

add_executable(exe src/exe.c)
target_link_libraries(exe PRIVATE stg)
target_link_libraries(exe PRIVATE http)

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
