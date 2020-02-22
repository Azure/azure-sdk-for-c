# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(BUILD_CURL_TRANSPORT "Build internal http transport implementation with CURL for HTTP Pipeline" OFF)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
elseif(DEFINED ENV{VCPKG_INSTALLATION_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_INSTALLATION_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()

if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()

project(az LANGUAGES C)
enable_testing ()

include(eng/cmake/global_compile_options.txt)

add_subdirectory(sdk/core/core)

add_subdirectory(sdk/platform/http_client/nohttp)
# Adding trasnport implementation for curl
# Users can still build Core and SDK client without depending on a HTPP transport implementation
if(BUILD_CURL_TRANSPORT)
  add_subdirectory(sdk/platform/http_client/curl)
endif()

add_subdirectory(sdk/keyvault/keyvault)
add_subdirectory(sdk/platform/noplatform)
add_subdirectory(sdk/platform/posix)
add_subdirectory(sdk/platform/win32)
add_subdirectory(sdk/storage/blobs)

if(NOT DEFINED ENV{AZ_SDK_C_NO_SAMPLES})
  # current samples require an HTTP transport implementation.
  # disable adding samples if building CURL transport is disabled
  # Remove this condition if implementing another http transport and link that with samples
  if(BUILD_CURL_TRANSPORT)
    add_subdirectory(sdk/keyvault/keyvault/samples)
    add_subdirectory(sdk/storage/blobs/samples)
  endif()
endif()
