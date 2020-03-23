# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake-modules")

option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(BUILD_CURL_TRANSPORT "Build internal http transport implementation with CURL for HTTP Pipeline" OFF)
option(UNIT_TESTING "Build unit test projects" OFF)
option(UNIT_TESTING_MOCK_ENABLED "wrap PAL functions with mock implementation for tests" OFF)

#enable mock functions with link option -ld
if(UNIT_TESTING_MOCK_ENABLED)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMOCK_ENABLED")
endif()

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
# Adding transport implementation for curl
# Users can still build Core and SDK client without depending on a HTPP transport implementation
if(BUILD_CURL_TRANSPORT)
  add_subdirectory(sdk/platform/http_client/curl)
endif()

# SDK Clients and tests
add_subdirectory(sdk/storage/blobs)
add_subdirectory(sdk/iot/core)
add_subdirectory(sdk/iot/hub)
add_subdirectory(sdk/iot/provisioning)

# PAL
add_subdirectory(sdk/platform/noplatform)
add_subdirectory(sdk/platform/posix)
add_subdirectory(sdk/platform/win32)

# User can disable samples generation by setting env variable AZ_SDK_C_NO_SAMPLES
if(NOT DEFINED ENV{AZ_SDK_C_NO_SAMPLES})
  add_subdirectory(sdk/samples/keyvault/keyvault)
  add_subdirectory(sdk/samples/keyvault/keyvault/samples)
  add_subdirectory(sdk/storage/blobs/samples)
endif()

# default for Unit testing with cmocka is OFF, however, this will be ON on CI and tests must
# past before commiting changes
if (UNIT_TESTING)
  add_subdirectory(sdk/core/core/test/cmocka)
  add_subdirectory(sdk/storage/blobs/test/cmocka)
  add_subdirectory(sdk/iot/hub/tests/cmocka)
  add_subdirectory(sdk/iot/provisioning/tests/cmocka)

  if(NOT DEFINED ENV{AZ_SDK_C_NO_SAMPLES})
    add_subdirectory(sdk/samples/keyvault/keyvault/test/cmocka)
  endif()
endif()
