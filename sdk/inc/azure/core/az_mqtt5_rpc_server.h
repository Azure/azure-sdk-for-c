// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_rpc_server. You use the RPC server to receive commands.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_RPC_SERVER_H
#define _az_MQTT5_RPC_SERVER_H

#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/core/_az_cfg_prefix.h>

#define AZ_MQTT5_RPC_SERVER_MINIMUM_TIMEOUT_SECONDS 10

/**
 * @brief The MQTT5 RPC Server.
 *
 */
typedef struct az_mqtt5_rpc_server az_mqtt5_rpc_server;

/**
 * @brief Event types for the MQTT5 RPC Server.
 *
 */
enum az_event_type_mqtt5_rpc_server
{
  /**
   * @brief Event representing the application finishing processing the command.
   *
   */
  AZ_EVENT_MQTT5_RPC_SERVER_EXECUTION_FINISH = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT5, 21),
  /**
   * @brief Event representing the rpc server requesting the execution of a command by the
   * application.
   */
  AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT5, 22),
  /**
   * @brief Event representing the rpc server receiving a command that it will not handle. Sent to
   * the application to deal with as desired.
   */
  AZ_EVENT_RPC_SERVER_UNHANDLED_COMMAND = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT5, 23)
};

/**
 * @brief The command that is currently waiting to be executed
 */
typedef struct
{
  az_mqtt5_property_string response_topic_property;
  az_mqtt5_property_binarydata correlation_data_property;
} az_mqtt5_rpc_server_pending_command;

/**
 * @brief MQTT5 RPC Server options.
 *
 */
typedef struct
{
  int8_t sub_qos;
  int8_t response_qos;
  az_span sub_topic;
  // name/type of command handled by this subclient
  az_span command_name;
  az_span model_id;

} az_mqtt5_rpc_server_options;

/**
 * @brief Data used by the rpc server policy that must be allocated by the application.
 */
typedef struct az_mqtt5_rpc_server_data
{
  az_mqtt5_property_bag property_bag;

  struct
  {
    /**
     * @brief the message id of the pending subscribe for the command topic
     */
    int32_t _az_mqtt5_rpc_server_pending_sub_id;
    _az_event_pipeline_timer rpc_server_timer;
    uint32_t retry_after_seconds;
    az_mqtt5_rpc_server_pending_command pending_command;
  } _internal;

} az_mqtt5_rpc_server_data;

/**
 * @brief The MQTT5 RPC Server.
 *
 */
struct az_mqtt5_rpc_server
{
  struct
  {
    /**
     * @brief RPC Server policy for the MQTT5 RPC Server.
     *
     */
    _az_hfsm rpc_server_policy;

    _az_event_client subclient;

    az_mqtt5_connection* connection;

    az_mqtt5_rpc_server_data rpc_server_data;

    /**
     * @brief Options for the MQTT5 RPC Server.
     *
     */
    az_mqtt5_rpc_server_options options;
  } _internal;
};

typedef enum
{
  // Default, unset value
  AZ_MQTT5_RPC_STATUS_UNKNOWN = 0,

  // Service success codes
  AZ_MQTT5_RPC_STATUS_OK = 200,
  // AZ_MQTT5_RPC_STATUS_ACCEPTED = 202,
  // AZ_MQTT5_RPC_STATUS_NO_CONTENT = 204,

  // Service error codes
  AZ_MQTT5_RPC_STATUS_BAD_REQUEST = 400,
  AZ_MQTT5_RPC_STATUS_UNAUTHORIZED = 401,
  AZ_MQTT5_RPC_STATUS_FORBIDDEN = 403,
  AZ_MQTT5_RPC_STATUS_NOT_FOUND = 404,
  AZ_MQTT5_RPC_STATUS_NOT_ALLOWED = 405,
  AZ_MQTT5_RPC_STATUS_NOT_CONFLICT = 409,
  AZ_MQTT5_RPC_STATUS_PRECONDITION_FAILED = 412,
  AZ_MQTT5_RPC_STATUS_REQUEST_TOO_LARGE = 413,
  AZ_MQTT5_RPC_STATUS_UNSUPPORTED_TYPE = 415,
  AZ_MQTT5_RPC_STATUS_THROTTLED = 429,
  AZ_MQTT5_RPC_STATUS_CLIENT_CLOSED = 499,
  AZ_MQTT5_RPC_STATUS_SERVER_ERROR = 500,
  AZ_MQTT5_RPC_STATUS_BAD_GATEWAY = 502,
  AZ_MQTT5_RPC_STATUS_SERVICE_UNAVAILABLE = 503,
  AZ_MQTT5_RPC_STATUS_TIMEOUT = 504,
} az_mqtt5_rpc_status;

// Event data types

/**
 * @brief Event data for #AZ_EVENT_MQTT5_RPC_SERVER_EXECUTION_FINISH.
 */
typedef struct az_mqtt5_rpc_server_execution_data
{
  az_span correlation_id;
  az_span response_topic;
  az_mqtt5_rpc_status status;
  az_span response;
  az_span error_message;
} az_mqtt5_rpc_server_execution_data;

/**
 * @brief Event data for #AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND.
 */
typedef struct
{
  az_span correlation_id;
  az_span response_topic;
  az_span command_name;
  az_span request_data;
} az_mqtt5_rpc_server_command_data;

AZ_NODISCARD az_result az_mqtt5_rpc_server_register(az_mqtt5_rpc_server* client);

/**
 * @brief Initializes an MQTT5 RPC Server.
 *
 * @param[out] client The az_mqtt5_rpc_server to initialize.
 * @param[in] connection The az_mqtt5_connection to use for the RPC Server.
 * @param[in] options Any az_mqtt5_rpc_server_options to use for the RPC Server.
 * @param[in] rpc_server_data The allocated az_mqtt5_rpc_server_data to use for the RPC Server.
 *
 * @return #az_result
 */
AZ_NODISCARD az_result az_rpc_server_init(
    az_mqtt5_rpc_server* client,
    az_mqtt5_connection* connection,
    az_mqtt5_rpc_server_options* options,
    az_mqtt5_rpc_server_data* rpc_server_data);

/**
 * @brief Triggers an AZ_EVENT_MQTT5_RPC_SERVER_EXECUTION_FINISH event from the application
 *
 * @param[in] client The az_mqtt5_rpc_server to use.
 * @param[in] data The information for the execution response
 *
 * @return #az_result
 */
AZ_NODISCARD az_result az_mqtt5_rpc_server_execution_finish(
    az_mqtt5_rpc_server* client,
    az_mqtt5_rpc_server_execution_data* data);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_RPC_SERVER_H
