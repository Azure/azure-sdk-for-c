# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_iot_samples LANGUAGES C)

set(CMAKE_C_STANDARD 99)

find_package(OpenSSL CONFIG)
if(NOT OpenSSL_FOUND)
  find_package(OpenSSL REQUIRED)
endif()

# Azure IoT Samples Library
add_library (az_iot_samples
  ${CMAKE_CURRENT_LIST_DIR}/src/sample_sas_utility.c
)

target_include_directories (az_iot_samples
  PUBLIC
    ${az_iot_samples_SOURCE_DIR}/src
)

# Internal deps
target_link_libraries(az_iot_samples
  PUBLIC
    az::iot::hub
)

# External deps
target_link_libraries(az_iot_samples
  PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)

add_library (az::iot::samples ALIAS az_iot_samples)
