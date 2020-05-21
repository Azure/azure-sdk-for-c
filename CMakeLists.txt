# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake-modules")

option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(UNIT_TESTING "Build unit test projects" OFF)
option(UNIT_TESTING_MOCKS "wrap PAL functions with mock implementation for tests" OFF)
option(TRANSPORT_PAHO "Build IoT Samples with Paho MQTT support" OFF)
option(PRECONDITIONS "Build SDK with preconditions enabled" ON)
option(LOGGING "Build SDK with logging support" ON)
set(PLATFORM "AUTO" CACHE STRING "Build SDK with the specific platform layer implementation")
set(HTTP_TRANSPORT "NONE" CACHE STRING "Build with HTTP transport implementation")


# disable preconditions when it's set to OFF
if (NOT PRECONDITIONS)
  add_compile_definitions(AZ_NO_PRECONDITION_CHECKING)
endif()

if (NOT LOGGING)
  add_compile_definitions(AZ_NO_LOGGING)
endif()

# enable mock functions with link option -ld
if(UNIT_TESTING_MOCKS)
  add_compile_definitions(_az_MOCK_ENABLED)
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

# List of projects that generate coverage
# This write empty makes sure that if file is already there, we replace it for an empty one
# Then each project will APPEND to this file
# At the end of cmake generate, this file will list the targets for code cov
file(WRITE ${CMAKE_BINARY_DIR}/coverage_targets.txt "")


add_subdirectory(sdk/core/az_core)

# SDK Clients and tests
add_subdirectory(sdk/storage/blobs)
add_subdirectory(sdk/iot/common)
add_subdirectory(sdk/iot/hub)
add_subdirectory(sdk/iot/provisioning)

#PAL
if(PLATFORM STREQUAL "AUTO")
  if (WIN32)
    set(PLATFORM "WIN32")
  elseif(UNIX)
    set(PLATFORM "POSIX")
  else()
    set(PLATFORM "NONE")
  endif()
endif()

if(PLATFORM STREQUAL "WIN32")
  add_subdirectory(sdk/platform/win32)
  set(PLATFORM_LIB az_win32)
elseif(PLATFORM STREQUAL "POSIX")
  add_subdirectory(sdk/platform/posix)
  set(PLATFORM_LIB az_posix)
elseif(PLATFORM STREQUAL "NONE")
  add_subdirectory(sdk/platform/noplatform)
  set(PLATFORM_LIB az_noplatform)
else()
  set(PLATFORM_LIB ${PLATFORM})
endif()

#HTTP Transport
if(HTTP_TRANSPORT STREQUAL "AUTO")
  if (WIN32 OR UNIX)
    set(HTTP_TRANSPORT "CURL")
  else()
    set(HTTP_TRANSPORT "NONE")
  endif()
endif()

if(HTTP_TRANSPORT STREQUAL "CURL")
  add_subdirectory(sdk/platform/http_client/curl)
  set(HTTP_TRANSPORT_LIB az_curl)
elseif(HTTP_TRANSPORT STREQUAL "NONE")
  add_subdirectory(sdk/platform/http_client/nohttp)
  set(HTTP_TRANSPORT_LIB az_nohttp)
else()
  set(HTTP_TRANSPORT_LIB ${HTTP_TRANSPORT})
endif()

# User can disable samples generation by setting env variable AZ_SDK_C_NO_SAMPLES
if(NOT DEFINED ENV{AZ_SDK_C_NO_SAMPLES})
  add_subdirectory(sdk/samples/keyvault/keyvault)
  add_subdirectory(sdk/samples/keyvault/keyvault/samples)
  add_subdirectory(sdk/storage/blobs/samples)
  if(TRANSPORT_PAHO)
    add_subdirectory(sdk/iot/provisioning/samples)
    add_subdirectory(sdk/iot/hub/samples)
  endif()
endif()

# default for Unit testing with cmocka is OFF, however, this will be ON on CI and tests must
# pass before commiting changes
if (UNIT_TESTING)
  add_subdirectory(sdk/core/az_core/test)
  add_subdirectory(sdk/storage/blobs/test)
  add_subdirectory(sdk/iot/common/tests)
  add_subdirectory(sdk/iot/hub/tests)
  add_subdirectory(sdk/iot/provisioning/tests)

  if(NOT DEFINED ENV{AZ_SDK_C_NO_SAMPLES})
    add_subdirectory(sdk/samples/keyvault/keyvault/test)
  endif()
endif()
