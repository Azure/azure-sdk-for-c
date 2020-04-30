# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_iot_hub_samples LANGUAGES C)

set(CMAKE_C_STANDARD 99)

find_package(eclipse-paho-mqtt-c CONFIG)
if(NOT eclipse-paho-mqtt-c_FOUND)
  find_package(eclipse-paho-mqtt-c REQUIRED)
endif()

find_package(OpenSSL CONFIG)
if(NOT OpenSSL_FOUND)
  find_package(OpenSSL REQUIRED)
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/c2d)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/methods)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/telemetry)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/twin)

