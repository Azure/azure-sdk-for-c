// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt_rpc_server. You use the RPC server to receive commands.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT_RPC_SERVER_H
#define _az_MQTT_RPC_SERVER_H

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/az_mqtt_connection.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief The MQTT RPC Server.
 *
 */
typedef struct az_mqtt_rpc_server az_mqtt_rpc_server;

/**
 * @brief Event types for the MQTT RPC Server.
 *
 */
enum az_event_type_mqtt_rpc_server
{
  /**
   * @brief Event representing the application finishing processing the command.
   *
   */
  AZ_EVENT_MQTT_RPC_SERVER_EXECUTION_FINISH = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT, 19),
  // AZ_MQTT_SERVER_EVENT_REGISTER_REQ = _az_MAKE_EVENT(_az_FACILITY_IOT, 16),
  AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT, 20),
};

typedef struct 
{
  az_span correlation_id;
  az_span response_topic;

} az_mqtt_rpc_server_pending_command;


/**
 * @brief MQTT RPC Server options.
 *
 */
typedef struct
{
  int8_t sub_qos;

  int8_t response_qos;

  az_span sub_topic;

  az_mqtt_rpc_server_pending_command pending_command;

  // name/type of command handled by this subclient (needed?)
  az_span command_handled;

  /**
   * @brief the message id of the pending subscribe for the command topic
  */
  int32_t _az_mqtt_rpc_server_pending_sub_id;

  /**
   * @brief The client id for the MQTT connection. REQUIRED if disable_sdk_connection_management is
   * false.
   *
   */
  // az_span client_id_buffer;


} az_mqtt_rpc_server_options;

/**
 * @brief The MQTT RPC Server.
 *
 */
struct az_mqtt_rpc_server
{
  struct
  {
    /**
     * @brief RPC Server policy for the MQTT RPC Server.
     *
     */
    _az_hfsm rpc_server_policy;

    _az_event_client subclient;

    az_mqtt_connection* connection;

    /**
     * @brief Policy collection.
     *
     */
    // _az_event_policy_collection policy_collection;

    /**
     * @brief MQTT policy.
     *
     */
    // _az_mqtt_policy mqtt_policy;

    /**
     * @brief Event pipeline for the MQTT connection.
     *
     */
    // _az_event_pipeline event_pipeline;

    /**
     * @brief Options for the MQTT RPC Server.
     *
     */
    az_mqtt_rpc_server_options options;
  } _internal;
};

typedef enum
{
  // Default, unset value
  AZ_MQTT_RPC_STATUS_UNKNOWN = 0,

  // Service success codes
  AZ_MQTT_RPC_STATUS_OK = 200,
  // AZ_MQTT_RPC_STATUS_ACCEPTED = 202,
  // AZ_MQTT_RPC_STATUS_NO_CONTENT = 204,

  // Service error codes
  AZ_MQTT_RPC_STATUS_BAD_REQUEST = 400,
  AZ_MQTT_RPC_STATUS_UNAUTHORIZED = 401,
  AZ_MQTT_RPC_STATUS_FORBIDDEN = 403,
  AZ_MQTT_RPC_STATUS_NOT_FOUND = 404,
  AZ_MQTT_RPC_STATUS_NOT_ALLOWED = 405,
  AZ_MQTT_RPC_STATUS_NOT_CONFLICT = 409,
  AZ_MQTT_RPC_STATUS_PRECONDITION_FAILED = 412,
  AZ_MQTT_RPC_STATUS_REQUEST_TOO_LARGE = 413,
  AZ_MQTT_RPC_STATUS_UNSUPPORTED_TYPE = 415,
  AZ_MQTT_RPC_STATUS_THROTTLED = 429,
  AZ_MQTT_RPC_STATUS_CLIENT_CLOSED = 499,
  AZ_MQTT_RPC_STATUS_SERVER_ERROR = 500,
  AZ_MQTT_RPC_STATUS_BAD_GATEWAY = 502,
  AZ_MQTT_RPC_STATUS_SERVICE_UNAVAILABLE = 503,
  AZ_MQTT_RPC_STATUS_TIMEOUT = 504,
} az_mqtt_rpc_status;

// Event data types
typedef struct az_mqtt_rpc_server_execution_data
{
  az_span correlation_id;
  az_span response_topic;
  az_mqtt_rpc_status status;
  az_span response;
  az_span error_message;
} az_mqtt_rpc_server_execution_data;

AZ_NODISCARD az_result az_mqtt_rpc_server_register(
    az_mqtt_rpc_server* client);

AZ_NODISCARD az_result az_rpc_server_init(
    az_mqtt_rpc_server* client,
    az_mqtt_connection* connection,
    az_mqtt_rpc_server_options* options);

AZ_NODISCARD az_result az_mqtt_rpc_server_execution_finish(
    az_mqtt_rpc_server* client,
    az_mqtt_rpc_server_execution_data* data);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT_RPC_SERVER_H
