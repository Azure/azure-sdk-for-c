# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

if (TRANSPORT_PAHO)

cmake_minimum_required (VERSION 3.10)

project (az_iot_samples_common LANGUAGES C)

set(CMAKE_C_STANDARD 99)

find_package(eclipse-paho-mqtt-c CONFIG)
if(NOT eclipse-paho-mqtt-c_FOUND)
  find_package(eclipse-paho-mqtt-c REQUIRED)
endif()

find_package(OpenSSL CONFIG)
if(NOT OpenSSL_FOUND)
  find_package(OpenSSL REQUIRED)
endif()

# Azure IoT Samples Library
add_library (az_iot_samples_common
  ${CMAKE_CURRENT_LIST_DIR}/src/sample_sas_utility.c
)

target_include_directories (az_iot_samples_common
  PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/inc
)

# Internal deps
target_link_libraries(az_iot_samples_common
  PUBLIC
    az::iot::hub
    az::iot::provisioning
)

# External deps
target_link_libraries(az_iot_samples_common
  PUBLIC
    eclipse-paho-mqtt-c::paho-mqtt3cs-static
    OpenSSL::SSL
    OpenSSL::Crypto
)

add_library (az::iot::samples::common ALIAS az_iot_samples_common)

add_subdirectory(hub)
add_subdirectory(provisioning)

endif() # TRANSPORT_PAHO

