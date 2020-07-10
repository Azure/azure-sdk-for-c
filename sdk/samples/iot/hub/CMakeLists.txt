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
add_executable (paho_iot_hub_c2d_example
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_hub_c2d_example.c
)

# SDK deps
target_link_libraries(paho_iot_hub_c2d_example
  PRIVATE
    az::iot::hub
)

# External deps
target_link_libraries(paho_iot_hub_c2d_example
  PRIVATE
    eclipse-paho-mqtt-c::paho-mqtt3cs-static
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Methods Sample
add_executable (paho_iot_hub_methods_example
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_hub_methods_example.c
)

# SDK deps
target_link_libraries(paho_iot_hub_methods_example
  PRIVATE
    az::iot::hub
)

# External deps
target_link_libraries(paho_iot_hub_methods_example
  PRIVATE
    eclipse-paho-mqtt-c::paho-mqtt3cs-static
    OpenSSL::SSL
    OpenSSL::Crypto
)

# PnP Sample
add_executable (paho_iot_hub_pnp_example
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_hub_pnp_example.c
)

# SDK deps
target_link_libraries(paho_iot_hub_pnp_example
  PRIVATE
    az::iot::hub
)

# External deps
target_link_libraries(paho_iot_hub_pnp_example
  PRIVATE
    eclipse-paho-mqtt-c::paho-mqtt3cs-static
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Telemetry (Certificates) Sample
add_executable (paho_iot_hub_telemetry_example
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_hub_telemetry_example.c
)

# SDK deps
target_link_libraries(paho_iot_hub_telemetry_example
  PRIVATE
    az::iot::hub
)

# External deps
target_link_libraries(paho_iot_hub_telemetry_example
  PRIVATE
    eclipse-paho-mqtt-c::paho-mqtt3cs-static
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Telemetry (SAS) Sample
add_executable (paho_iot_hub_sas_telemetry_example
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_hub_sas_telemetry_example.c
  ${CMAKE_CURRENT_LIST_DIR}/src/sample_sas_utility.c
)

# SDK deps
target_link_libraries(paho_iot_hub_sas_telemetry_example
  PRIVATE
    az::iot::hub
)

# External deps
target_link_libraries(paho_iot_hub_sas_telemetry_example
  PRIVATE
    eclipse-paho-mqtt-c::paho-mqtt3cs-static
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Twin Sample
add_executable (paho_iot_hub_twin_example
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_hub_twin_example.c
)

# SDK deps
target_link_libraries(paho_iot_hub_twin_example
  PRIVATE
    az::iot::hub
)

# External deps
target_link_libraries(paho_iot_hub_twin_example
  PRIVATE
    eclipse-paho-mqtt-c::paho-mqtt3cs-static
    OpenSSL::SSL
    OpenSSL::Crypto
)
