# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

if (BUILD_PAHO_TRANSPORT)

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

add_executable (
  paho_iot_hub_twin_example 
  src/paho_iot_hub_twin_example.c
  )

target_link_libraries(
  paho_iot_hub_twin_example PRIVATE
  az_iot_hub
  az_iot_core
  az_core
  )

target_link_libraries(
  paho_iot_hub_twin_example PRIVATE
  eclipse-paho-mqtt-c::paho-mqtt3cs-static
  OpenSSL::SSL
  OpenSSL::Crypto
  )

endif()
