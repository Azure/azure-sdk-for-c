// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_rpc_server. You use the RPC server to receive commands.
 *
 * @note The state diagram is in sdk/docs/core/mqtt_rpc_server.puml
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_RPC_SERVER_H
#define _az_MQTT5_RPC_SERVER_H

#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_mqtt5_rpc.h>
#include <azure/core/az_mqtt5_rpc_server_codec.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/core/_az_cfg_prefix.h>

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
  AZ_MQTT5_EVENT_RPC_SERVER_EXECUTE_COMMAND_RSP = _az_MAKE_EVENT(_az_FACILITY_RPC_SERVER, 1),
  /**
   * @brief Event representing the RPC server requesting the execution of a command by the
   * application.
   */
  AZ_MQTT5_EVENT_RPC_SERVER_EXECUTE_COMMAND_REQ = _az_MAKE_EVENT(_az_FACILITY_RPC_SERVER, 2)
};

/**
 * @brief The MQTT5 RPC Server.
 *
 */
typedef struct az_mqtt5_rpc_server
{
  struct
  {
    /**
     * @brief RPC Server HFSM for the MQTT5 RPC Server.
     *
     */
    _az_hfsm rpc_server_hfsm;

    /**
     * @brief The subclient used by the MQTT5 RPC Server.
     */
    _az_event_client subclient;

    /**
     * @brief The MQTT5 connection linked to the MQTT5 RPC Server.
     */
    az_mqtt5_connection* connection;

    /**
     * @brief Timeout in seconds for subscribing acknowledgement.
     */
    int32_t subscribe_timeout_in_seconds;

    /**
     * @brief The property bag used by the RPC server for sending response messages.
     */
    az_mqtt5_property_bag property_bag;

    /**
     * @brief the message id of the pending subscribe for the command topic.
     */
    int32_t pending_subscription_id;

    /**
     * @brief timer used for the subscribe of the command topic.
     */
    _az_event_pipeline_timer rpc_server_timer;

    /**
     * @brief The subscription topic to use for the RPC Server.
     */
    az_span subscription_topic;

    /**
     * @brief #az_mqtt5_rpc_server_codec associated with this server.
     */
    az_mqtt5_rpc_server_codec* rpc_server_codec;
  } _internal;
} az_mqtt5_rpc_server;

// Event data types

/**
 * @brief Event data for #AZ_MQTT5_EVENT_RPC_SERVER_EXECUTE_COMMAND_RSP.
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
 * @brief Event data for #AZ_MQTT5_EVENT_RPC_SERVER_EXECUTE_COMMAND_REQ.
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
  /**
   * @brief The service id of the request.
   */
  az_span service_id;
  /**
   * @brief The command name of the request.
   */
  az_span command_name;
} az_mqtt5_rpc_server_execution_req_event_data;

/**
 * @brief Starts the MQTT5 RPC Server.
 *
 * @param[in] client The #az_mqtt5_rpc_server to start.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_server_register(az_mqtt5_rpc_server* client);

/**
 * @brief Initializes an MQTT5 RPC Server.
 *
 * @param[out] client The #az_mqtt5_rpc_server to initialize.
 * @param[in] rpc_server_codec The #az_mqtt5_rpc_server_codec to initialize and use within the RPC
 * Server.
 * @param[in] connection The #az_mqtt5_connection to use for the RPC Server.
 * @param[in] property_bag The application allocated #az_mqtt5_property_bag to use for the
 * RPC Server.
 * @param[in] subscription_topic The application allocated #az_span to use for the subscription
 * topic.
 * @param[in] model_id The model id to use for the subscription topic. May be AZ_SPAN_EMPTY if not
 * used in the subscription topic.
 * @param[in] client_id The client id to use for the subscription topic. May be AZ_SPAN_EMPTY if not
 * used in the subscription topic.
 * @param[in] subscribe_timeout_in_seconds Timeout in seconds for subscribing acknowledgement (must
 * be > 0).
 * @param[in] options Any #az_mqtt5_rpc_server_codec_options to use for the RPC Server.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_server_init(
    az_mqtt5_rpc_server* client,
    az_mqtt5_rpc_server_codec* rpc_server_codec,
    az_mqtt5_connection* connection,
    az_mqtt5_property_bag property_bag,
    az_span subscription_topic,
    az_span model_id,
    az_span client_id,
    int32_t subscribe_timeout_in_seconds,
    az_mqtt5_rpc_server_codec_options* options);

/**
 * @brief Triggers an AZ_MQTT5_EVENT_RPC_SERVER_EXECUTE_COMMAND_RSP event from the application.
 *
 * @note This should be called from the application when it has finished processing the command,
 * regardless of whether that is a successful execution, a failed execution, a timeout, etc.
 *
 * @param[in] client The #az_mqtt5_rpc_server to use.
 * @param[in] data The information for the execution response.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_server_execution_finish(
    az_mqtt5_rpc_server* client,
    az_mqtt5_rpc_server_execution_rsp_event_data* data);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_RPC_SERVER_H
