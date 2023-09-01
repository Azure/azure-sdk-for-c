// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_rpc_client. You use the RPC client to send commands.
 *
 * @note The state diagram for this policy is in sdk/docs/core/rpc_client.puml
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_RPC_CLIENT_H
#define _az_MQTT5_RPC_CLIENT_H

#include <stdio.h>

#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_mqtt5_rpc.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/core/_az_cfg_prefix.h>

// ~~~~~~~~~~~~~~~~~~~~ Codec Public API ~~~~~~~~~~~~~~~~~
/**
 * @brief MQTT5 RPC Client options.
 *
 */
typedef struct
{
  /**
   * @brief timeout in seconds for subscribing (must be > 0)
   */
  uint32_t subscribe_timeout_in_seconds;

  /**
   * @brief timeout in seconds for publishing (must be > 0)
   */
  uint32_t publish_timeout_in_seconds;

} az_mqtt5_rpc_client_options;

/**
 * @brief The MQTT5 RPC Client.
 *
 */
typedef struct az_mqtt5_rpc_client
{
  struct
  {
    az_span client_id;
    az_span model_id;
    az_span command_name;
    az_span response_topic_buffer;
    az_span subscription_topic;
    az_span request_topic_buffer;
    /**
     * @brief Options for the MQTT5 RPC Client.
     */
    az_mqtt5_rpc_client_options options;
  } _internal;
} az_mqtt5_rpc_client;

/**
 * @brief Generates the subscription topic for this RPC Client
 *
 * @param[in] client The az_mqtt5_rpc_client to use.
 * @param[out] out_subscription_topic The buffer to write the subscription topic to.
 * @param[out] out_topic_length The length of the subscription topic.
 *
 * @note The out_subscription_topic will return the same length that it was passed in with. You can
 * use the out_topic_length to slice it to length. ex. az_span_slice(out_subscription_topic, 0,
 * *out_topic_length);
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_rpc_client_get_subscription_topic(
    az_mqtt5_rpc_client* client,
    az_span out_subscription_topic,
    int32_t* out_topic_length);

/**
 * @brief Generates the response topic for this RPC_client
 *
 * @param[in] client The az_mqtt5_rpc_client to use.
 * @param[in] server_client_id The client id of the server to send the request to.
 * @param[out] out_response_topic The buffer to write the response topic to. Must be large enough to
 * hold the server_client_id and the subscription_topic.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_rpc_client_get_response_topic(
    az_mqtt5_rpc_client* client,
    az_span server_client_id,
    az_span out_response_topic);

/**
 * @brief Generates the request topic for this RPC_client
 *
 * @param[in] client The az_mqtt5_rpc_client to use.
 * @param[in] server_client_id The client id of the server to send the request to.
 * @param[out] out_request_topic The buffer to write the request topic to.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_rpc_client_get_request_topic(
    az_mqtt5_rpc_client* client,
    az_span server_client_id,
    az_span out_request_topic);

/**
 * @brief Returns the default options for the MQTT5 RPC Client.
 *
 * @return An #az_mqtt5_rpc_client_options object with default values.
 */
AZ_NODISCARD az_mqtt5_rpc_client_options az_mqtt5_rpc_client_options_default();

/**
 * @brief Initializes an MQTT5 RPC Client.
 *
 * @param[out] client The az_mqtt5_rpc_client to initialize.
 * @param[in] client_id The client id to use for the response topic.
 * @param[in] model_id The model id to use for the topics.
 * @param[in] command_name The command name to use for the topics.
 * @param[in] response_topic_buffer The application allocated az_span to use for the response topic
 * @param[in] request_topic_buffer The application allocated az_span to use for the request topic
 * @param[in] subscribe_topic_buffer The application allocated az_span to use for the subscription
 * topic
 * @param[in] options Any az_mqtt5_rpc_client_options to use for the RPC Client.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_rpc_client_init(
    az_mqtt5_rpc_client* client,
    az_span client_id,
    az_span model_id,
    az_span command_name,
    az_span response_topic_buffer,
    az_span request_topic_buffer,
    az_span subscribe_topic_buffer,
    az_mqtt5_rpc_client_options* options);

// ~~~~~~~~~~~~~~~~~~~~ RPC Client Policy API ~~~~~~~~~~~~~~~~~

/**
 * @brief Event types for the MQTT5 RPC Client.
 *
 */
enum az_event_type_mqtt5_rpc_client
{
  /**
   * @brief Event representing the application requesting to subscribe to the response topic so
   * commands can be invoked.
   */
  AZ_EVENT_MQTT5_RPC_CLIENT_SUB_REQ = _az_MAKE_EVENT(_az_FACILITY_RPC_CLIENT, 1),
  /**
   * @brief Event representing the MQTT5 RPC Client being ready to receive invoke requests from the
   * application.
   */
  AZ_EVENT_MQTT5_RPC_CLIENT_READY_IND = _az_MAKE_EVENT(_az_FACILITY_RPC_CLIENT, 2),
  /**
   * @brief Event representing the application requesting to send a command.
   */
  AZ_EVENT_MQTT5_RPC_CLIENT_INVOKE_REQ = _az_MAKE_EVENT(_az_FACILITY_RPC_CLIENT, 3),
  /**
   * @brief Event representing the RPC client receiving a command response and sending it to the
   * application
   */
  AZ_EVENT_MQTT5_RPC_CLIENT_RSP = _az_MAKE_EVENT(_az_FACILITY_RPC_CLIENT, 4),
  /**
   * @brief Event representing the RPC client getting an error from the broker when invoking the
   * request, or receiving a command response, but there was an error parsing it. It is then sent to
   * the application. This event indicates the failure is from the client/broker, not the server
   *
   * @note The data on this event will only have best effort decoding and will be at minimum
   * partial (if not completely corrupted)
   */
  AZ_EVENT_MQTT5_RPC_CLIENT_ERROR_RSP = _az_MAKE_EVENT(_az_FACILITY_RPC_CLIENT, 5),
  /**
   * @brief Event representing the application requesting the RPC client to unsubscribe from the
   * response topic
   */
  AZ_EVENT_MQTT5_RPC_CLIENT_UNSUB_REQ = _az_MAKE_EVENT(_az_FACILITY_RPC_CLIENT, 6)
};

/**
 * @brief The type represents the various #az_result success and error conditions specific to the
 * IoT clients within the SDK.
 */
enum az_result_rpc_client
{
  // === RPC Client error codes ===
  /// Another publish is already in progress and puback hasn't been received yet
  AZ_ERROR_RPC_PUB_IN_PROGRESS = _az_RESULT_MAKE_ERROR(_az_FACILITY_RPC_CLIENT, 1),
};

/**
 * @brief The MQTT5 RPC Client Policy.
 *
 */
typedef struct az_mqtt5_rpc_client_policy
{
  struct
  {
    /**
     * @brief RPC Client policy for the MQTT5 RPC Client.
     *
     */
    _az_hfsm rpc_client_hfsm;

    /**
     * @brief The subclient used by the MQTT5 RPC Client.
     */
    _az_event_client subclient;

    /**
     * @brief The MQTT5 connection linked to the MQTT5 RPC Client.
     */
    az_mqtt5_connection* connection;

    /**
     * @brief the message id of the pending subscribe for the response topic
     */
    int32_t pending_subscription_id;

    /**
     * @brief the message id of the pending publish for the request
     */
    int32_t pending_pub_id;

    /**
     * @brief the message id of the pending publish for the request
     */
    az_span pending_pub_correlation_id;

    /**
     * @brief timer used for the subscribe and publishes
     */
    _az_event_pipeline_timer rpc_client_timer;

    /**
     * @brief az_mqtt5_rpc_client associated with this policy
     */
    az_mqtt5_rpc_client* rpc_client;

    /**
     * @brief The property bag used by the RPC client policy for sending request messages
     */
    az_mqtt5_property_bag property_bag;

  } _internal;
} az_mqtt5_rpc_client_policy;

// Event data types

/**
 * @brief Event data for #AZ_EVENT_MQTT5_RPC_CLIENT_INVOKE_REQ.
 */
typedef struct az_mqtt5_rpc_client_invoke_req_event_data
{
  /**
   * @brief The correlation id of the request.
   */
  az_span correlation_id;

  /**
   * @brief The content type of the request.
   */
  az_span content_type;

  /**
   * @brief The payload of the request.
   */
  az_span request_payload;

  /**
   * @brief The message id of the request to correlate with pubacks.
   */
  int32_t mid;

  /**
   * @brief The client id of the server to send the request to.
   */
  az_span rpc_server_client_id;

} az_mqtt5_rpc_client_invoke_req_event_data;

/**
 * @brief Event data for #AZ_EVENT_MQTT5_RPC_CLIENT_RSP or #AZ_EVENT_MQTT5_RPC_CLIENT_ERROR_RSP.
 */
typedef struct az_mqtt5_rpc_client_rsp_event_data
{
  /**
   * @brief The correlation id of the response.
   */
  az_span correlation_id;

  /**
   * @brief The error message of the response.
   */
  az_span error_message;

  /**
   * @brief The status of the response.
   */
  az_mqtt5_rpc_status status;

  /**
   * @brief The content type of the response.
   */
  az_span content_type;

  /**
   * @brief The payload of the response.
   */
  az_span response_payload;

} az_mqtt5_rpc_client_rsp_event_data;

/**
 * @brief Triggers an AZ_EVENT_MQTT5_RPC_CLIENT_INVOKE_REQ event from the application
 *
 * @note This should be called from the application when wants to request a command to be invoked.
 *
 * @param[in] client The az_mqtt5_rpc_client_policy to use.
 * @param[in] data The information for the execution request
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The event was triggered successfully.
 * @retval #AZ_ERROR_HFSM_INVALID_STATE If called when the policy hasn't been asked to subscribe yet
 * or is still subscribing.
 * @retval #AZ_ERROR_NOT_SUPPORTED if the client is not connected.
 * @retval Other on other failures creating/sending the request message.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_client_invoke_begin(
    az_mqtt5_rpc_client_policy* client,
    az_mqtt5_rpc_client_invoke_req_event_data* data);

/**
 * @brief Triggers an AZ_EVENT_MQTT5_RPC_CLIENT_SUB_REQ event from the application
 *
 * @note This should be called from the application to subscribe to the response topic. The RPC
 * Client must be subscribed before commands can be invoked.
 *
 * @param[in] client The az_mqtt5_rpc_client_policy to use.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The event was triggered successfully or the client is already subscribing.
 * @retval #AZ_ERROR_HFSM_INVALID_STATE If called when the policy is already subscribed - the
 * application doesn't need to wait for the AZ_EVENT_MQTT5_RPC_CLIENT_READY_IND event to start
 * sending commands in this case.
 * @retval #AZ_ERROR_NOT_SUPPORTED if the client is not connected.
 * @retval Other on other failures creating/sending the subscribe message.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_client_subscribe_begin(az_mqtt5_rpc_client_policy* client);

/**
 * @brief Triggers an AZ_EVENT_MQTT5_RPC_CLIENT_UNSUB_REQ event from the application
 *
 * @note This should be called from the application to unsubscribe to the response topic. This will
 * prevent the application from invoking commands unless it subscribes again. This may be used if
 * the application doesn't want to recieve responses anymore.
 *
 * @param[in] client The az_mqtt5_rpc_client_policy to use.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The event was triggered successfully.
 * @retval #AZ_ERROR_NOT_SUPPORTED if the client is not connected.
 * @retval Other on other failures creating/sending the unsubscribe message.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_client_unsubscribe_begin(az_mqtt5_rpc_client_policy* client);

/**
 * @brief Initializes an MQTT5 RPC Client Policy.
 *
 * @param[out] client The az_mqtt5_rpc_client_policy to initialize.
 * @param[out] rpc_client The az_mqtt5_rpc_client to initialize and use within the RPC Client
 * Policy.
 * @param[in] connection The az_mqtt5_connection to use for the RPC Client.
 * @param[in] property_bag The application allocated az_mqtt5_property_bag to use for the
 * RPC Client.
 * @param[in] client_id The client id to use for the response topic.
 * @param[in] model_id The model id to use for the topics.
 * @param[in] command_name The command name to use for the topics.
 * @param[in] response_topic_buffer The application allocated az_span to use for the response topic
 * @param[in] request_topic_buffer The application allocated az_span to use for the request topic
 * @param[in] subscribe_topic_buffer The application allocated az_span to use for the subscription
 * topic
 * @param[in] options Any az_mqtt5_rpc_server_options to use for the RPC Server.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_rpc_client_policy_init(
    az_mqtt5_rpc_client_policy* client,
    az_mqtt5_rpc_client* rpc_client,
    az_mqtt5_connection* connection,
    az_mqtt5_property_bag property_bag,
    az_span client_id,
    az_span model_id,
    az_span command_name,
    az_span response_topic_buffer,
    az_span request_topic_buffer,
    az_span subscribe_topic_buffer,
    az_mqtt5_rpc_client_options* options);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_RPC_CLIENT_H
