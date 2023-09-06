// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_rpc_server. You use the RPC server to receive commands.
 *
 * @note The state diagram for this HFSM is in sdk/docs/core/rpc_server.puml
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
#ifndef AZ_MQTT5_RPC_QOS
#define AZ_MQTT5_RPC_QOS 1
#endif

// ~~~~~~~~~~~~~~~~~~~~ Codec Public API ~~~~~~~~~~~~~~~~~

/**
 * @brief MQTT5 RPC Server options.
 *
 */
typedef struct
{
  /**
   * @brief timeout in seconds for subscribing
   */
  uint32_t subscribe_timeout_in_seconds;

} az_mqtt5_rpc_server_options;

/**
 * @brief The MQTT5 RPC Server.
 *
 */
typedef struct az_mqtt5_rpc_server
{
  struct
  {
    az_span model_id;
    az_span client_id;
    az_span command_name;
    /**
     * @brief The topic to subscribe to for commands
     */
    az_span subscription_topic;

    /**
     * @brief Options for the MQTT5 RPC Server.
     */
    az_mqtt5_rpc_server_options options;
  } _internal;
} az_mqtt5_rpc_server;

/**
 * @brief Initializes a RPC server options object with default values.
 *
 * @return An #az_mqtt5_rpc_server_options object with default values.
 */
AZ_NODISCARD az_mqtt5_rpc_server_options az_mqtt5_rpc_server_options_default();

AZ_NODISCARD az_result az_rpc_server_get_subscription_topic(az_mqtt5_rpc_server* client, az_span out_subscription_topic);

AZ_NODISCARD az_result az_rpc_server_init(
    az_mqtt5_rpc_server* client,
    az_span model_id, az_span client_id, az_span command_name,
    az_span subscription_topic,
    az_mqtt5_rpc_server_options* options);


// ~~~~~~~~~~~~~~~~~~~~ RPC Server Policy API ~~~~~~~~~~~~~~~~~

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
 * @brief The MQTT5 RPC Server Policy.
 *
 */
typedef struct az_mqtt5_rpc_server_policy
{
  struct
  {
    /**
     * @brief RPC Server hfsm for the MQTT5 RPC Server Policy.
     *
     */
    _az_hfsm rpc_server_hfsm;

    /**
     * @brief The subclient used by the MQTT5 RPC Server Policy.
     */
    _az_event_client subclient;

    /**
     * @brief The MQTT5 connection linked to the MQTT5 RPC Server Policy.
     */
    az_mqtt5_connection* connection;

    /**
     * @brief The property bag used by the RPC server policy for sending response messages
     */
    az_mqtt5_property_bag property_bag;

    /**
     * @brief the message id of the pending subscribe for the command topic
     */
    int32_t pending_subscription_id;

    /**
     * @brief timer used for the subscribe of the command topic
     */
    _az_event_pipeline_timer rpc_server_timer;

    /**
     * @brief az_mqtt5_rpc_server associated with this policy
    */
    az_mqtt5_rpc_server* rpc_server;
  } _internal;
} az_mqtt5_rpc_server_policy;

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
   * @brief The request topic to make sure the right RPC server sends the response.
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
 * @brief Starts the MQTT5 RPC Server Policy.
 *
 * @param[in] client The az_mqtt5_rpc_server_policy to start.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_server_register(az_mqtt5_rpc_server_policy* client);

/**
 * @brief Initializes an MQTT5 RPC Server Policy.
 *
 * @param[out] client The az_mqtt5_rpc_server_policy to initialize.
 * @param[in] rpc_server The az_mqtt5_rpc_server to initialize and use within the RPC Server Policy.
 * @param[in] connection The az_mqtt5_connection to use for the RPC Server Policy.
 * @param[in] property_bag The application allocated az_mqtt5_property_bag to use for the
 * RPC Server Policy.
 * @param[in] subscription_topic The application allocated az_span to use for the subscription topic
 * @param[in] model_id The model id to use for the subscription topic.
 * @param[in] client_id The client id to use for the subscription topic.
 * @param[in] command_name The command name to use for the subscription topic.
 * @param[in] options Any az_mqtt5_rpc_server_options to use for the RPC Server.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_rpc_server_policy_init(
    az_mqtt5_rpc_server_policy* client,
    az_mqtt5_rpc_server* rpc_server,
    az_mqtt5_connection* connection,
    az_mqtt5_property_bag property_bag,
    az_span subscription_topic,
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
 * @param[in] client The az_mqtt5_rpc_server_policy to use.
 * @param[in] data The information for the execution response
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_server_execution_finish(
    az_mqtt5_rpc_server_policy* client,
    az_mqtt5_rpc_server_execution_rsp_event_data* data);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_RPC_SERVER_H
