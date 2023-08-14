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

/**
 * @brief The default timeout in seconds for subscribing.
 */
#define AZ_MQTT5_RPC_SERVER_DEFAULT_TIMEOUT_SECONDS 10
/**
 * @brief The default QOS to use for subscribing/publishing.
 */
#define AZ_MQTT5_RPC_DEFAULT_QOS 1

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
  AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND_RSP = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT5, 21),
  /**
   * @brief Event representing the RPC server requesting the execution of a command by the
   * application.
   */
  AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND_REQ = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT5, 22)
};

/**
 * @brief MQTT5 RPC Server options.
 *
 */
typedef struct
{
  /**
   * @brief QOS to use for subscribing
   */
  int8_t sub_qos;
  /**
   * @brief QOS to use for sending responses
   */
  int8_t response_qos;

} az_mqtt5_rpc_server_options;

/**
 * @brief Memory used by the RPC server policy that must be allocated by the application.
 */
typedef struct az_mqtt5_rpc_server_memory
{
  /**
   * @brief The property bag used by the RPC server policy for sending response messages
   */
  az_mqtt5_property_bag property_bag;
  /**
   * @brief The topic to subscribe to for commands
   */
  az_span sub_topic;

  struct
  {
    /**
     * @brief the message id of the pending subscribe for the command topic
     */
    int32_t _az_mqtt5_rpc_server_pending_sub_id;
    /**
     * @brief timer used for the subscribe of the command topic
     */
    _az_event_pipeline_timer rpc_server_timer;
    /**
     * @brief timeout in seconds for subscribing
     */
    uint32_t subscribe_timeout_in_seconds;
  } _internal;

} az_mqtt5_rpc_server_memory;

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

    /**
     * @brief The subclient used by the MQTT5 RPC Server.
     */
    _az_event_client subclient;

    /**
     * @brief The MQTT5 connection linked to the MQTT5 RPC Server.
     */
    az_mqtt5_connection* connection;

    /**
     * @brief The memory used by the MQTT5 RPC Server that is allocated by the application.
     */
    az_mqtt5_rpc_server_memory rpc_server_memory;

    /**
     * @brief Options for the MQTT5 RPC Server.
     */
    az_mqtt5_rpc_server_options options;
  } _internal;
};

/**
 * @brief The MQTT5 RPC Server status codes to include on the response.
 */
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
 * @brief Event data for #AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND_RSP.
 */
typedef struct az_mqtt5_rpc_server_execution_rsp_event_data
{
  /**
   * @brief The correlation id of the command.
   */
  az_span correlation_id;
  /**
   * @brief The request topic to make sure the right rpc server sends the response.
   */
  az_span request_topic;
  /**
   * @brief The topic to send the response to.
   */
  az_span response_topic;
  /**
   * @brief The status code of the execution.
   */
  az_mqtt5_rpc_status status;
  /**
   * @brief The response payload.
   * @note Will be AZ_SPAN_EMPTY when the status is an error status.
   */
  az_span response;
  /**
   * @brief The error message if the status is an error status.
   * @note Will be AZ_SPAN_EMPTY when the status is not an error status.
   *      Can be AZ_SPAN_EMPTY on error as well.
   */
  az_span error_message;
  /**
   * @brief The content type of the response.
   */
  az_span content_type;
} az_mqtt5_rpc_server_execution_rsp_event_data;

/**
 * @brief Event data for #AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND_REQ.
 */
typedef struct
{
  /**
   * @brief The correlation id of the command.
   */
  az_span correlation_id;
  /**
   * @brief The topic to send the response to.
   */
  az_span response_topic;
  /**
   * @brief The request topic.
   */
  az_span request_topic;
  /**
   * @brief The command request payload.
   */
  az_span request_data;
  /**
   * @brief The content type of the request.
   */
  az_span content_type;
} az_mqtt5_rpc_server_execution_req_event_data;

/**
 * @brief Starts the MQTT5 RPC Server.
 *
 * @param[in] client The az_mqtt5_rpc_server to start.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_server_register(az_mqtt5_rpc_server* client);

/**
 * @brief Initializes a RPC server options object with default values.
 *
 * @return An #az_mqtt5_rpc_server_options object with default values.
 */
AZ_NODISCARD az_mqtt5_rpc_server_options az_mqtt5_rpc_server_options_default();

/**
 * @brief Initializes an MQTT5 RPC Server.
 *
 * @param[out] client The az_mqtt5_rpc_server to initialize.
 * @param[in] connection The az_mqtt5_connection to use for the RPC Server.
 * @param[in] rpc_server_memory The application allocated az_mqtt5_rpc_server_memory to use for the
 * RPC Server.
 * @param[in] model_id The model id to use for the subscription topic.
 * @param[in] client_id The client id to use for the subscription topic.
 * @param[in] command_name The command name to use for the subscription topic.
 * @param[in] options Any az_mqtt5_rpc_server_options to use for the RPC Server.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_rpc_server_init(
    az_mqtt5_rpc_server* client,
    az_mqtt5_connection* connection,
    az_mqtt5_rpc_server_memory* rpc_server_memory,
    az_span model_id,
    az_span client_id,
    az_span command_name,
    az_mqtt5_rpc_server_options* options);

/**
 * @brief Triggers an AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND_RSP event from the application
 *
 * @note This should be called from the application when it has finished processing the command,
 * regardless of whether that is a successful execution, a failed execution, a timeout, etc.
 *
 * @param[in] client The az_mqtt5_rpc_server to use.
 * @param[in] data The information for the execution response
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_server_execution_finish(
    az_mqtt5_rpc_server* client,
    az_mqtt5_rpc_server_execution_rsp_event_data* data);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_RPC_SERVER_H
