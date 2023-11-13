# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_mqtt5_samples LANGUAGES C)

set(CMAKE_C_STANDARD 99)

if (TRANSPORT_MOSQUITTO)
  # Azure MQTT5 State Machine RPC Server Sample
  add_executable (mosquitto_rpc_server_sample
    ${CMAKE_CURRENT_LIST_DIR}/mosquitto_rpc_server_sample.c
    ${CMAKE_CURRENT_LIST_DIR}/mosquitto_rpc_server_sample_json_parser.c
  )

  target_include_directories(mosquitto_rpc_server_sample
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}
      ${CMAKE_SOURCE_DIR}/sdk/src/azure/core/
  )

  target_link_libraries(mosquitto_rpc_server_sample
    PRIVATE
    az_core
  )

  create_map_file(mosquitto_rpc_server_sample mosquitto_rpc_server_sample.map)

  # Azure MQTT5 State Machine RPC Client Sample
  add_executable (mosquitto_rpc_client_sample
    ${CMAKE_CURRENT_LIST_DIR}/mosquitto_rpc_client_sample.c
  )

  target_include_directories(mosquitto_rpc_client_sample
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}
      ${CMAKE_SOURCE_DIR}/sdk/src/azure/core/
  )

  target_link_libraries(mosquitto_rpc_client_sample
    PRIVATE
    uuid
    az_core
  )

  create_map_file(mosquitto_rpc_client_sample mosquitto_rpc_client_sample.map)

  # Azure MQTT5 State Machine RPC Client & Server Sample
  add_executable (mosquitto_multiple_rpc_sample
    ${CMAKE_CURRENT_LIST_DIR}/mosquitto_multiple_rpc_sample.c
  )

  target_include_directories(mosquitto_multiple_rpc_sample
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}
      ${CMAKE_SOURCE_DIR}/sdk/src/azure/core/
  )

  target_link_libraries(mosquitto_multiple_rpc_sample
    PRIVATE
    uuid
    az_core
  )

  create_map_file(mosquitto_multiple_rpc_sample mosquitto_multiple_rpc_sample.map)
elseif (TRANSPORT_PAHO)
  # Azure MQTT5 State Machine RPC Server Sample
  add_executable (paho_rpc_server_sample
    ${CMAKE_CURRENT_LIST_DIR}/paho_rpc_server_sample.c
    ${CMAKE_CURRENT_LIST_DIR}/mosquitto_rpc_server_sample_json_parser.c
  )

  target_include_directories(paho_rpc_server_sample
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}
      ${CMAKE_SOURCE_DIR}/sdk/src/azure/core/
  )

  target_link_libraries(paho_rpc_server_sample
    PRIVATE
    az_core
  )

  create_map_file(paho_rpc_server_sample paho_rpc_server_sample.map)

  # Azure MQTT5 State Machine RPC Client Sample
  add_executable (paho_rpc_client_sample
    ${CMAKE_CURRENT_LIST_DIR}/paho_rpc_client_sample.c
  )

  target_include_directories(paho_rpc_client_sample
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}
      ${CMAKE_SOURCE_DIR}/sdk/src/azure/core/
  )

  target_link_libraries(paho_rpc_client_sample
    PRIVATE
    uuid
    az_core
  )

  create_map_file(paho_rpc_client_sample paho_rpc_client_sample.map)

  # Azure MQTT5 State Machine RPC Client & Server Sample
  add_executable (paho_multiple_rpc_sample
    ${CMAKE_CURRENT_LIST_DIR}/paho_multiple_rpc_sample.c
  )

  target_include_directories(paho_multiple_rpc_sample
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}
      ${CMAKE_SOURCE_DIR}/sdk/src/azure/core/
  )

  target_link_libraries(paho_multiple_rpc_sample
    PRIVATE
    uuid
    az_core
  )

  create_map_file(paho_multiple_rpc_sample paho_multiple_rpc_sample.map)

  # Azure MQTT5 State Machine Telemetry Producer Sample
  add_executable (paho_telemetry_producer_sample
    ${CMAKE_CURRENT_LIST_DIR}/paho_telemetry_producer_sample.c
  )

  target_include_directories(paho_telemetry_producer_sample
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}
      ${CMAKE_SOURCE_DIR}/sdk/src/azure/core/
  )

  target_link_libraries(paho_telemetry_producer_sample
    PRIVATE
    az_core
  )

  create_map_file(paho_telemetry_producer_sample paho_telemetry_producer_sample.map)

  # Azure MQTT5 State Machine Telemetry Consumer Sample
  add_executable (paho_telemetry_consumer_sample
    ${CMAKE_CURRENT_LIST_DIR}/paho_telemetry_consumer_sample.c
    ${CMAKE_CURRENT_LIST_DIR}/mosquitto_telemetry_consumer_sample_json_parser.c
  )

  target_include_directories(paho_telemetry_consumer_sample
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}
      ${CMAKE_SOURCE_DIR}/sdk/src/azure/core/
  )

  target_link_libraries(paho_telemetry_consumer_sample
    PRIVATE
    az_core
  )

  create_map_file(paho_telemetry_consumer_sample paho_telemetry_consumer_sample.map)
endif()
