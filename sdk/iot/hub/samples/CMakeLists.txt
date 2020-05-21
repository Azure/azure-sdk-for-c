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
  paho_iot_hub_c2d_example 
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_hub_c2d_example.c
  )

target_link_libraries(
  paho_iot_hub_c2d_example PRIVATE
  az_iot_hub
  az_iot_common
  az_core
)

# PAL
target_link_libraries(paho_iot_hub_c2d_example PRIVATE ${PLATFORM_LIB})

target_link_libraries(
  paho_iot_hub_c2d_example PRIVATE
  eclipse-paho-mqtt-c::paho-mqtt3cs-static
  OpenSSL::SSL
  OpenSSL::Crypto
  )

# Methods Sample
add_executable (
  paho_iot_hub_methods_example 
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_hub_methods_example.c
  )

target_link_libraries(
  paho_iot_hub_methods_example PRIVATE
  az_iot_hub
  az_iot_common
  az_core
  )

# PAL
target_link_libraries(paho_iot_hub_methods_example PRIVATE ${PLATFORM_LIB})

target_link_libraries(
  paho_iot_hub_methods_example PRIVATE
  eclipse-paho-mqtt-c::paho-mqtt3cs-static
  OpenSSL::SSL
  OpenSSL::Crypto
  )

# Telemetry Sample
add_executable (
  paho_iot_hub_telemetry_example 
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_hub_telemetry_example.c
  )

target_link_libraries(
  paho_iot_hub_telemetry_example PRIVATE
  az_iot_hub
  az_iot_common
  az_core
  )

# PAL
target_link_libraries(paho_iot_hub_telemetry_example PRIVATE ${PLATFORM_LIB})

target_link_libraries(
  paho_iot_hub_telemetry_example PRIVATE
  eclipse-paho-mqtt-c::paho-mqtt3cs-static
  OpenSSL::SSL
  OpenSSL::Crypto
  )

# Twin Sample
add_executable (
  paho_iot_hub_twin_example 
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_hub_twin_example.c
  )

target_link_libraries(
  paho_iot_hub_twin_example PRIVATE
  az_iot_hub
  az_iot_common
  az_core
  )

# PAL
target_link_libraries(paho_iot_hub_twin_example PRIVATE ${PLATFORM_LIB})

target_link_libraries(
  paho_iot_hub_twin_example PRIVATE
  eclipse-paho-mqtt-c::paho-mqtt3cs-static
  OpenSSL::SSL
  OpenSSL::Crypto
  )
