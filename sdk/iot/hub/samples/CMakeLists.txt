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

# C2D Sample
add_executable (
  iot_hub_c2d_sample 
  ${CMAKE_CURRENT_LIST_DIR}/src/iot_hub_c2d_sample.c
  )

target_link_libraries(
  iot_hub_c2d_sample PRIVATE
  az_iot_hub
  az_iot_core
  az_core
)

target_link_libraries(
  iot_hub_c2d_sample PRIVATE
  eclipse-paho-mqtt-c::paho-mqtt3cs-static
  OpenSSL::SSL
  OpenSSL::Crypto
  )

# Methods Sample
add_executable (
  iot_hub_methods_sample 
  ${CMAKE_CURRENT_LIST_DIR}/src/iot_hub_methods_sample.c
  )

target_link_libraries(
  iot_hub_methods_sample PRIVATE
  az_iot_hub
  az_iot_core
  az_core
  )

target_link_libraries(
  iot_hub_methods_sample PRIVATE
  eclipse-paho-mqtt-c::paho-mqtt3cs-static
  OpenSSL::SSL
  OpenSSL::Crypto
  )

# Telemetry Sample
add_executable (
  iot_hub_telemetry_sample 
  ${CMAKE_CURRENT_LIST_DIR}/src/iot_hub_telemetry_sample.c
  )

target_link_libraries(
  iot_hub_telemetry_sample PRIVATE
  az_iot_hub
  az_iot_core
  az_core
  )

target_link_libraries(
  iot_hub_telemetry_sample PRIVATE
  eclipse-paho-mqtt-c::paho-mqtt3cs-static
  OpenSSL::SSL
  OpenSSL::Crypto
  )

# Twin Sample
add_executable (
  iot_hub_twin_sample 
  ${CMAKE_CURRENT_LIST_DIR}/src/iot_hub_twin_sample.c
  )

target_link_libraries(
  iot_hub_twin_sample PRIVATE
  az_iot_hub
  az_iot_core
  az_core
  )

target_link_libraries(
  iot_hub_twin_sample PRIVATE
  eclipse-paho-mqtt-c::paho-mqtt3cs-static
  OpenSSL::SSL
  OpenSSL::Crypto
  )
