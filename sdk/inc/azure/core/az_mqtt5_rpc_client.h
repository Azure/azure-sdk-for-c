// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_rpc_client. You use the RPC client to send commands.
 *
 * @note The state diagram is in sdk/docs/core/resources/mqtt_rpc_client.puml
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
#include <azure/core/az_mqtt5_rpc_client_codec.h>
#include <azure/core/az_mqtt5_request.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_event_policy_collection_internal.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Event types for the MQTT5 RPC Client.
 *
 */
enum az_mqtt5_event_type_rpc_client
{
  /**
   * @brief Event representing the application requesting to subscribe to the response topic so
   * commands can be invoked.
   */
  AZ_MQTT5_EVENT_RPC_CLIENT_SUB_REQ = _az_MAKE_EVENT(_az_FACILITY_RPC_CLIENT, 1),
  /**
   * @brief Event representing the MQTT5 RPC Client being ready to receive invoke requests from the
   * application.
   */
  AZ_MQTT5_EVENT_RPC_CLIENT_READY_IND = _az_MAKE_EVENT(_az_FACILITY_RPC_CLIENT, 2),
  /**
   * @brief Event representing the application requesting to send a command.
   */
  AZ_MQTT5_EVENT_RPC_CLIENT_INVOKE_REQ = _az_MAKE_EVENT(_az_FACILITY_RPC_CLIENT, 3),
  /**
   * @brief Event representing the RPC client receiving a command response from the server and
   * sending it to the application.
   */
  AZ_MQTT5_EVENT_RPC_CLIENT_RSP = _az_MAKE_EVENT(_az_FACILITY_RPC_CLIENT, 4),
  /**
   * @brief Event representing the RPC client getting an error from the broker when invoking the
   * request, or receiving a command response, but there was an error parsing it. It is then sent to
   * the application. This event indicates the failure is from the client/broker, not the server.
   *
   * @note The data on this event will only have best effort decoding and will be at minimum
   * partial (if not completely corrupted).
   */
  AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP = _az_MAKE_EVENT(_az_FACILITY_RPC_CLIENT, 5),
  /**
   * @brief Event representing the application requesting the RPC client to unsubscribe from the
   * response topic.
   */
  AZ_MQTT5_EVENT_RPC_CLIENT_UNSUB_REQ = _az_MAKE_EVENT(_az_FACILITY_RPC_CLIENT, 6),

  /**
   * @brief Event representing the application requesting the RPC client to remove the request and provide the request's memory pointers to free.
   */
  AZ_MQTT5_EVENT_RPC_CLIENT_REMOVE_REQ = _az_MAKE_EVENT(_az_FACILITY_RPC_CLIENT, 7),
};

/**
 * @brief The type represents the various #az_result error conditions specific to the
 * RPC Client.
 */
enum az_result_rpc_client
{
  // === RPC Client error codes ===
  /**
   * @brief Another publish is already in progress and the puback hasn't been received yet.
   */
  AZ_ERROR_RPC_PUB_IN_PROGRESS = _az_RESULT_MAKE_ERROR(_az_FACILITY_RPC_CLIENT, 1),
};

/**
 * @brief The MQTT5 RPC Client.
 *
 */
typedef struct az_mqtt5_rpc_client
{
  struct
  {
    /**
     * @brief RPC Client hfsm for the MQTT5 RPC Client.
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
     * @brief The policy collection that all MQTT Request policies will be a part of.
    */
    _az_event_policy_collection request_policy_collection;

    /**
     * @brief Timeout in seconds for subscribing (must be > 0).
     */
    int32_t subscribe_timeout_in_seconds;

    /**
     * @brief Timeout in seconds for publishing (must be > 0).
     */
    int32_t publish_timeout_in_seconds;

    /**
     * @brief The message id of the pending subscribe for the response topic.
     */
    int32_t pending_subscription_id;

    /**
     * @brief The message id of the pending publish for the request.
     */
    int32_t pending_pub_id;

    /**
     * @brief the correlation id of the pending publish for the request.
     */
    az_span pending_pub_correlation_id;

    /**
     * @brief The application allocated #az_span to use for the response topic.
     */
    az_span response_topic_buffer;

    /**
     * @brief The application allocated #az_span to use for the request topic.
     */
    az_span request_topic_buffer;

    /**
     * @brief The subscription topic to use for the RPC Client.
     */
    az_span subscription_topic;

    /**
     * @brief timer used for the subscribe and publishes.
     */
    _az_event_pipeline_timer rpc_client_timer;

    /**
     * @brief #az_mqtt5_rpc_client_codec associated with this client.
     */
    az_mqtt5_rpc_client_codec* rpc_client_codec;

    /**
     * @brief The property bag used by the RPC client for sending request messages.
     */
    az_mqtt5_property_bag property_bag;

    /**
     * @brief The maximum number of pending requests allowed at a time.
     */
    size_t max_pending_requests;

  } _internal;
} az_mqtt5_rpc_client;

// Event data types

/**
 * @brief Event data for #AZ_MQTT5_EVENT_RPC_CLIENT_INVOKE_REQ.
 */
typedef struct az_mqtt5_rpc_client_invoke_req_event_data
{
  /**
   * @brief The correlation id of the request.
   */
  az_span correlation_id;

  /**
   * @brief The content type of the request. It can be AZ_SPAN_EMPTY.
   */
  az_span content_type;

  /**
   * @brief The payload of the request.
   */
  az_span request_payload;

  /**
   * @brief The application allocated memory to use for the lifetime of the request.
  */
  az_mqtt5_request* request_memory;

  /**
   * @brief The command name of the request.
   */
  az_span command_name;

  /**
   * @brief The client id of the server to send the request to.
   */
  az_span rpc_server_client_id;

  /**
   * @brief Timeout in seconds for request completion (must be > 0).
   */
  int32_t timeout_s;

} az_mqtt5_rpc_client_invoke_req_event_data;

/**
 * @brief Event data for #AZ_MQTT5_EVENT_RPC_CLIENT_RSP or #AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP.
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
 * @brief Event data for #AZ_MQTT5_EVENT_RPC_CLIENT_REMOVE_REQ.
 */
typedef struct az_mqtt5_rpc_client_remove_req_event_data
{
  /**
   * @brief The correlation id of the request to be free'd. Will be set to the correlation id span that should have it's memory free'd.
  */
  az_span* correlation_id;
  /**
   * @brief The policy to be free'd. The event should be created with this empty, and the event handler will set it to the policy that should be free'd
  */
  az_mqtt5_request** policy;
} az_mqtt5_rpc_client_remove_req_event_data;

/**
 * @brief Triggers an #AZ_MQTT5_EVENT_RPC_CLIENT_INVOKE_REQ event from the application.
 *
 * @note This should be called from the application when it wants to request that a command is
 * invoked.
 *
 * @param[in] client The #az_mqtt5_rpc_client to use.
 * @param[in] data The information for the execution request.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The event was triggered successfully.
 * @retval #AZ_ERROR_HFSM_INVALID_STATE If called when the client hasn't been asked to subscribe
 * yet, is still subscribing, or is in a faulted state.
 * @retval #AZ_ERROR_NOT_SUPPORTED if the client is not connected.
 * @retval Other on other failures creating/sending the request message.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_client_invoke_begin(
    az_mqtt5_rpc_client* client,
    az_mqtt5_rpc_client_invoke_req_event_data* data);

/**
 * @brief Triggers an #AZ_MQTT5_EVENT_RPC_CLIENT_REMOVE_REQ event from the application.
 * 
 * @note This should be called from the application when it wants to remove a request from the RPC Client.
 * 
 * @param[in] client The #az_mqtt5_rpc_client to use.
 * @param[in] data A #az_mqtt5_rpc_client_remove_req_event_data object with the correlation id of the
 * request to remove. On the return of this function, this object will have a pointer to the memory to
 * free for the correlation id and a pointer to the policy memory to free (these have been allocated
 * by the application on creation of the request).
 * 
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The event was triggered successfully.
 * @retval #AZ_ERROR_NOT_SUPPORTED if the client is not connected.
 * @retval #AZ_ERROR_ITEM_NOT_FOUND if the request was not found.
 * @retval Other on other failures getting the data to free/removing the request policy.
*/
AZ_NODISCARD az_result az_mqtt5_rpc_client_remove_request(
    az_mqtt5_rpc_client* client,
    az_mqtt5_rpc_client_remove_req_event_data* data);

/**
 * @brief Triggers an #AZ_MQTT5_EVENT_RPC_CLIENT_SUB_REQ event from the application.
 *
 * @note This should be called from the application to subscribe to the response topic. The RPC
 * Client must be subscribed before commands can be invoked.
 *
 * @param[in] client The #az_mqtt5_rpc_client to use.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The event was triggered successfully or the client is already subscribing.
 * @retval #AZ_ERROR_HFSM_INVALID_STATE If called when the client is already subscribed - the
 * application doesn't need to wait for the #AZ_MQTT5_EVENT_RPC_CLIENT_READY_IND event to start
 * sending commands in this case.
 * @retval #AZ_ERROR_NOT_SUPPORTED if the client is not connected.
 * @retval Other on other failures creating/sending the subscribe message.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_client_subscribe_begin(az_mqtt5_rpc_client* client);

/**
 * @brief Triggers an #AZ_MQTT5_EVENT_RPC_CLIENT_UNSUB_REQ event from the application.
 *
 * @note This should be called from the application to unsubscribe to the response topic. This will
 * prevent the application from invoking commands unless it subscribes again. This may be used if
 * the application doesn't want to receive responses anymore.
 *
 * @param[in] client The #az_mqtt5_rpc_client to use.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The event was triggered successfully.
 * @retval #AZ_ERROR_NOT_SUPPORTED if the client is not connected.
 * @retval Other on other failures creating/sending the unsubscribe message.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_client_unsubscribe_begin(az_mqtt5_rpc_client* client);

/**
 * @brief Initializes an MQTT5 RPC Client.
 *
 * @param[out] client The #az_mqtt5_rpc_client to initialize.
 * @param[out] rpc_client_codec The #az_mqtt5_rpc_client_codec to initialize and use within the RPC
 * Client.
 * @param[in] connection The #az_mqtt5_connection to use for the RPC Client.
 * @param[in] property_bag The application allocated #az_mqtt5_property_bag to use for the
 * RPC Client.
 * @param[in] client_id The client id to use for the response topic.
 * @param[in] model_id The model id to use for the topics.
 * @param[in] response_topic_buffer The application allocated #az_span to use for the response
 * topic.
 * @param[in] request_topic_buffer The application allocated #az_span to use for the request topic.
 * @param[in] subscribe_topic_buffer The application allocated #az_span to use for the subscription
 * topic.
 * @param[in] correlation_id_buffer The application allocated #az_span to use for the correlation id
 * during publish.
 * @param[in] subscribe_timeout_in_seconds Timeout in seconds for subscribing (must be > 0).
 * @param[in] publish_timeout_in_seconds Timeout in seconds for publishing (must be > 0).
 * @param[in] max_pending_requests The maximum number of pending requests at a time.
 * @param[in] options Any #az_mqtt5_rpc_client_codec_options to use for the RPC Client or NULL to
 * use the defaults.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_client_init(
    az_mqtt5_rpc_client* client,
    az_mqtt5_rpc_client_codec* rpc_client_codec,
    az_mqtt5_connection* connection,
    az_mqtt5_property_bag property_bag,
    az_span client_id,
    az_span model_id,
    az_span response_topic_buffer,
    az_span request_topic_buffer,
    az_span subscribe_topic_buffer,
    az_span correlation_id_buffer,
    int32_t subscribe_timeout_in_seconds,
    int32_t publish_timeout_in_seconds,
    size_t max_pending_requests,
    az_mqtt5_rpc_client_codec_options* options);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_RPC_CLIENT_H
