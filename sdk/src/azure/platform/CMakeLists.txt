# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project(az_platform LANGUAGES C)
set(CMAKE_C_STANDARD 99)

if(AZ_PLATFORM_IMPL STREQUAL "WIN32")
  # build windows platform
  add_library(az_platform_win32 STATIC
    ${CMAKE_CURRENT_LIST_DIR}/az_platform_win32.c
  )

  target_link_libraries(az_platform_win32
    PRIVATE
      az_core
  )
elseif(AZ_PLATFORM_IMPL STREQUAL "POSIX")
  # build linux platform
  add_library(az_platform_posix STATIC
      ${CMAKE_CURRENT_LIST_DIR}/az_platform_posix.c
  )

  target_link_libraries(az_platform_posix
    PRIVATE
      az_core
  )
else()
  #noplatform
  add_library(az_platform_stub STATIC
    ${CMAKE_CURRENT_LIST_DIR}/az_platform_stub.c
  )

  target_link_libraries(az_platform_stub
    PRIVATE
      az_core
  )
endif()

# HTTP Client
add_library (
  az_http_stub
    STATIC
      ${CMAKE_CURRENT_LIST_DIR}/az_http_stub.c
)

target_link_libraries(az_http_stub PRIVATE az_core)

# make sure that users can consume the project as a library.
add_library (az::nohttp ALIAS az_http_stub)

# Curl Platform
if (TRANSPORT_CURL)
  set(CURL_MIN_REQUIRED_VERSION 7.1) #Min curl version to support CURLOPT_HTTPHEADER option
  find_package(CURL ${CURL_MIN_REQUIRED_VERSION} CONFIG) 
  if(NOT CURL_FOUND)
    find_package(CURL ${CURL_MIN_REQUIRED_VERSION} REQUIRED)
  endif()

  add_library (
    az_http_curl
      STATIC
      ${CMAKE_CURRENT_LIST_DIR}/az_http_curl.c
  )

  target_link_libraries(az_http_curl PRIVATE az_core)

  # make sure that users can consume the project as a library.
  add_library (az::curl ALIAS az_http_curl)

  target_link_libraries(az_http_curl PRIVATE CURL::libcurl)

endif()
