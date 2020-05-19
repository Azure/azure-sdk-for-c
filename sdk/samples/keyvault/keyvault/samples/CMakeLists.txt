# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_keyvault_keys_samples LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# make sure libcurl is available for sample if compile option for libcurl is on
if(TRANSPORT_CURL)
  find_package(CURL ${CURL_MIN_REQUIRED_VERSION} CONFIG) 
  if(NOT CURL_FOUND)
    find_package(CURL ${CURL_MIN_REQUIRED_VERSION} REQUIRED)
  endif()
endif()

# make keys client example
add_executable (keys_client_example src/keys_client_example.c)

target_link_libraries(keys_client_example PRIVATE az_keyvault az_core)

# link libcurl impl when option is on
# When using a different HTTP transport adapter implementation, link it here instead of az_curl and libcurl
if(TRANSPORT_CURL)
  target_link_libraries(keys_client_example PRIVATE az_curl CURL::libcurl)
else()
  # linking default no http implementation to be able to compile and link sample.
  target_link_libraries(keys_client_example PRIVATE az_nohttp)
  # push compiler symbol so we can find out in Code about no http
  target_compile_definitions(keys_client_example PRIVATE AZ_NO_HTTP)
endif()

# PAL
target_link_libraries(keys_client_example PRIVATE ${PAL})
