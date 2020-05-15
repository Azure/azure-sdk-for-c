# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

if (TRANSPORT_PAHO)

cmake_minimum_required (VERSION 3.10)

project (az_iot_provisioning_samples LANGUAGES C)

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
  paho_iot_provisioning_example 
  src/paho_iot_provisioning_example.c
  )

target_link_libraries(
  paho_iot_provisioning_example PRIVATE
  az_iot_provisioning
  az_iot_common
  az_core
  )

# detect what platform impl to link automatically
if (WIN32)
  target_link_libraries(paho_iot_provisioning_example PRIVATE az_win32)
elseif(UNIX)
  target_link_libraries(paho_iot_provisioning_example PRIVATE az_posix)
else()
  target_link_libraries(paho_iot_provisioning_example PRIVATE az_noplatform)
endif()

target_link_libraries(
  paho_iot_provisioning_example PRIVATE
  eclipse-paho-mqtt-c::paho-mqtt3cs-static
  OpenSSL::SSL
  OpenSSL::Crypto
  )

endif()
