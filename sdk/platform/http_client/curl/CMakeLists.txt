# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

if (TRANSPORT_CURL)

cmake_minimum_required (VERSION 3.10)

project (az_curl LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CURL_MIN_REQUIRED_VERSION 7.1) #Min curl version to support CURLOPT_HTTPHEADER option
find_package(CURL ${CURL_MIN_REQUIRED_VERSION} CONFIG) 
if(NOT CURL_FOUND)
  find_package(CURL ${CURL_MIN_REQUIRED_VERSION} REQUIRED)
endif()

add_library (
  az_curl STATIC
  src/az_curl.c
  )

target_link_libraries(az_curl PUBLIC az_core)

# make sure that users can consume the project as a library.
add_library (az::curl ALIAS az_curl)

target_link_libraries(az_curl PRIVATE CURL::libcurl)

endif()
