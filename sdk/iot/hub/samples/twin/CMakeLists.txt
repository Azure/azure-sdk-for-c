# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

add_executable (
  iot_hub_twin_example 
  src/iot_hub_twin_example.c
  )

target_link_libraries(
  iot_hub_twin_example PRIVATE
  az_iot_hub
  az_iot_core
  az_core
  )

target_link_libraries(
  iot_hub_twin_example PRIVATE
  eclipse-paho-mqtt-c::paho-mqtt3cs-static
  OpenSSL::SSL
  OpenSSL::Crypto
  )
