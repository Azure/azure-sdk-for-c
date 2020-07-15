# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

if (TRANSPORT_PAHO)

cmake_minimum_required (VERSION 3.10)

project (az_iot_hub_samples LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# C2D Sample
add_executable (paho_iot_hub_c2d_example
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_hub_c2d_example.c
)

target_link_libraries(paho_iot_hub_c2d_example
  PRIVATE
    az::iot::samples::common
)

# Methods Sample
add_executable (paho_iot_hub_methods_example
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_hub_methods_example.c
)

target_link_libraries(paho_iot_hub_methods_example
  PRIVATE
    az::iot::samples::common
)

# PnP Sample
add_executable (paho_iot_hub_pnp_example
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_hub_pnp_example.c
)

target_link_libraries(paho_iot_hub_pnp_example
  PRIVATE
    az::iot::samples::common
)

# Telemetry (Certificates) Sample
add_executable (paho_iot_hub_telemetry_example
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_hub_telemetry_example.c
)

target_link_libraries(paho_iot_hub_telemetry_example
  PRIVATE
    az::iot::samples::common
)

# Telemetry (SAS) Sample
add_executable (paho_iot_hub_sas_telemetry_example
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_hub_sas_telemetry_example.c
)

target_link_libraries(paho_iot_hub_sas_telemetry_example
  PRIVATE
    az::iot::samples::common
)

# Twin Sample
add_executable (paho_iot_hub_twin_example
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_hub_twin_example.c
)

target_link_libraries(paho_iot_hub_twin_example
  PRIVATE
    az::iot::samples::common
)

endif() # TRANSPORT_PAHO
