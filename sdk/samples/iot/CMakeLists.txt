# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

if (TRANSPORT_PAHO)

cmake_minimum_required (VERSION 3.10)

project (az_iot_samples LANGUAGES C)

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
add_library (az_iot_sample_common
  ${CMAKE_CURRENT_LIST_DIR}/iot_sample_common.c
)

# Internal deps
target_link_libraries(az_iot_sample_common
  PUBLIC
    az::iot::hub
    az::iot::provisioning
)

# External deps
target_link_libraries(az_iot_sample_common
  PUBLIC
    eclipse-paho-mqtt-c::paho-mqtt3cs-static
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Workaround for linker warning LNK4098: defaultlib 'LIBCMTD' conflicts with use of other libs
if (MSVC)
    set_target_properties(az_iot_sample_common
        PROPERTIES
        LINK_FLAGS
        "/NODEFAULTLIB:libcmtd.lib"
        LINK_FLAGS_RELEASE
        "/NODEFAULTLIB:libcmt.lib"
    )
endif()

add_library (az::iot::sample::common ALIAS az_iot_sample_common)

# Azure IoT Samples Executables

# C2D Sample
add_executable (paho_iot_hub_c2d_sample
  ${CMAKE_CURRENT_LIST_DIR}/paho_iot_hub_c2d_sample.c
)

target_link_libraries(paho_iot_hub_c2d_sample
  PRIVATE
    az::iot::sample::common
)

# Methods Sample
add_executable (paho_iot_hub_methods_sample
  ${CMAKE_CURRENT_LIST_DIR}/paho_iot_hub_methods_sample.c
)

target_link_libraries(paho_iot_hub_methods_sample
  PRIVATE
    az::iot::sample::common
)

# PnP Sample
add_executable (paho_iot_hub_pnp_sample
  ${CMAKE_CURRENT_LIST_DIR}/paho_iot_hub_pnp_sample.c
)

target_link_libraries(paho_iot_hub_pnp_sample
  PRIVATE
    az::iot::sample::common
)

# PnP Component Sample
add_executable (paho_iot_hub_pnp_component_sample
  ${CMAKE_CURRENT_LIST_DIR}/pnp/pnp_mqtt_message.c
  ${CMAKE_CURRENT_LIST_DIR}/pnp/pnp_device_info_component.c
  ${CMAKE_CURRENT_LIST_DIR}/pnp/pnp_protocol.c
  ${CMAKE_CURRENT_LIST_DIR}/pnp/pnp_thermostat_component.c
  ${CMAKE_CURRENT_LIST_DIR}/paho_iot_hub_pnp_component_sample.c
)

# SDK deps
target_link_libraries(paho_iot_hub_pnp_component_sample
  PRIVATE
    az::iot::sample::common
)

target_include_directories(paho_iot_hub_pnp_component_sample
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Telemetry (Certificates) Sample
add_executable (paho_iot_hub_telemetry_sample
  ${CMAKE_CURRENT_LIST_DIR}/paho_iot_hub_telemetry_sample.c
)

target_link_libraries(paho_iot_hub_telemetry_sample
  PRIVATE
    az::iot::sample::common
)

# Telemetry (SAS) Sample
add_executable (paho_iot_hub_sas_telemetry_sample
  ${CMAKE_CURRENT_LIST_DIR}/paho_iot_hub_sas_telemetry_sample.c
)

target_link_libraries(paho_iot_hub_sas_telemetry_sample
  PRIVATE
    az::iot::sample::common
)

# Twin Sample
add_executable (paho_iot_hub_twin_sample
  ${CMAKE_CURRENT_LIST_DIR}/paho_iot_hub_twin_sample.c
)

target_link_libraries(paho_iot_hub_twin_sample
  PRIVATE
    az::iot::sample::common
)

# Provisioning (Certificates) Sample
add_executable (paho_iot_provisioning_sample
  ${CMAKE_CURRENT_LIST_DIR}/paho_iot_provisioning_sample.c
)

target_link_libraries(paho_iot_provisioning_sample
  PRIVATE
    az::iot::sample::common
)

# Provisioning (SAS) Sample
add_executable (paho_iot_provisioning_sas_sample
  ${CMAKE_CURRENT_LIST_DIR}/paho_iot_provisioning_sas_sample.c
)

target_link_libraries(paho_iot_provisioning_sas_sample
  PRIVATE
    az::iot::sample::common
)

# Workaround for linker warning LNK4098: defaultlib 'LIBCMTD' conflicts with use of other libs
if (MSVC)
    set_target_properties(
        paho_iot_hub_c2d_sample
        paho_iot_hub_methods_sample
        paho_iot_hub_pnp_component_sample
        paho_iot_hub_pnp_sample
        paho_iot_hub_telemetry_sample
        paho_iot_hub_sas_telemetry_sample
        paho_iot_hub_twin_sample
        paho_iot_provisioning_sample
        paho_iot_provisioning_sas_sample
        PROPERTIES
        LINK_FLAGS
        "/NODEFAULTLIB:libcmtd.lib"
        LINK_FLAGS_RELEASE
        "/NODEFAULTLIB:libcmt.lib"
    )
endif()

endif() # TRANSPORT_PAHO
