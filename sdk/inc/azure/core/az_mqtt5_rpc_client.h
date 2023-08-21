// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_rpc_client. You use the RPC client to send commands.
 *
 * @note The state diagram for this HFSM is in sdk/docs/core/rpc_client.puml
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_RPC_CLIENT_H
#define _az_MQTT5_RPC_CLIENT_H

#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/az_mqtt5_rpc.h>

#include <azure/core/_az_cfg_prefix.h>

// ~~~~~~~~~~~~~~~~~~~~ Codec Public API ~~~~~~~~~~~~~~~~~
/**
 * @brief The MQTT5 RPC Client.
 *
 */
typedef struct az_mqtt5_rpc_client az_mqtt5_rpc_client;

/**
 * @brief MQTT5 RPC Client options.
 *
 */
typedef struct
{
  /**
   * @brief QOS to use for subscribing
   */
  int8_t subscribe_qos;
  /**
   * @brief QOS to use for sending requests
   */
  int8_t request_qos;
  /**
   * @brief timeout in seconds for subscribing
   */
  uint32_t subscribe_timeout_in_seconds;

} az_mqtt5_rpc_client_options;

/**
 * @brief The MQTT5 RPC Client.
 *
 */
struct az_mqtt5_rpc_client
{
  az_span client_id;
  az_span model_id;
  az_span executor_client_id;
  az_span command_name;
  az_span response_topic;
  az_span request_topic;
  /**
   * @brief Options for the MQTT5 RPC Client.
   */
  az_mqtt5_rpc_client_options options;
};

AZ_NODISCARD az_result az_rpc_client_get_response_topic(az_mqtt5_rpc_client* client, az_span* out_response_topic);

AZ_NODISCARD az_result az_rpc_client_get_request_topic(az_mqtt5_rpc_client* client, az_span out_request_topic);

static int32_t ran = 1;

AZ_INLINE az_span az_rpc_client_generate_correlation_id()
{
  char corr_id_str[37];
  sprintf(corr_id_str, "%d", ran);
  ran++;
  return az_span_create_from_str(corr_id_str);
}

AZ_NODISCARD az_mqtt5_rpc_client_options az_mqtt5_rpc_client_options_default();

AZ_NODISCARD az_result az_rpc_client_init(
    az_mqtt5_rpc_client* client,
    az_span client_id,
    az_span model_id,
    az_span executor_client_id,
    az_span command_name,
    az_span response_topic_buffer,
    az_span request_topic_buffer,
    az_mqtt5_rpc_client_options* options);

// ~~~~~~~~~~~~~~~~~~~~ HFSM RPC Client API ~~~~~~~~~~~~~~~~~

/**
 * @brief The MQTT5 RPC Client hfsm.
 *
 */
typedef struct az_mqtt5_rpc_client_hfsm az_mqtt5_rpc_client_hfsm;

/**
 * @brief Event types for the MQTT5 RPC Client.
 *
 */
enum az_event_type_mqtt5_rpc_client
{
  /**
   * @brief Event representing the application requesting to send a command.
   *
   */
  AZ_EVENT_RPC_CLIENT_INVOKE_COMMAND_REQ = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT5, 23),
  /**
   * @brief Event representing the RPC client receiving a command response.
   */
  AZ_EVENT_RPC_CLIENT_COMMAND_RSP = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT5, 24)
};

struct az_mqtt5_rpc_client_hfsm
{
  struct
  {
    /**
     * @brief RPC Client policy for the MQTT5 RPC Client.
     *
     */
    _az_hfsm rpc_client_policy;

    /**
     * @brief The subclient used by the MQTT5 RPC Client.
     */
    _az_event_client subclient;

    /**
     * @brief The MQTT5 connection linked to the MQTT5 RPC Client.
     */
    az_mqtt5_connection* connection;

    /**
     * @brief the message id of the pending subscribe for the command topic
     */
    int32_t pending_subscription_id;

    /**
     * @brief timer used for the subscribe of the command topic
     */
    _az_event_pipeline_timer rpc_client_timer;

    /**
     * @brief az_mqtt5_rpc_client associated with this hfsm
    */
    az_mqtt5_rpc_client* rpc_client;

    /**
     * @brief The property bag used by the RPC client policy for sending response messages
     */
    az_mqtt5_property_bag property_bag;

  } _internal;
};

// Event data types

/**
 * @brief Event data for #AZ_EVENT_RPC_CLIENT_INVOKE_COMMAND_REQ.
 */
typedef struct az_mqtt5_rpc_client_command_req_event_data
{
  /**
   * @brief The correlation id of the command.
   */
  az_span correlation_id;

  /**
   * @brief The content type of the command.
   */
  az_span content_type;

  az_span request_payload;
} az_mqtt5_rpc_client_command_req_event_data;

/**
 * @brief Event data for #AZ_EVENT_RPC_CLIENT_COMMAND_RSP.
 */
typedef struct az_mqtt5_rpc_client_command_rsp_event_data
{
  /**
   * @brief The correlation id of the command.
   */
  az_span correlation_id;

  az_span error_message;
  az_mqtt5_rpc_status status;

  /**
   * @brief The content type of the command.
   */
  az_span content_type;

  az_span response_payload;

  bool parsing_failure;
} az_mqtt5_rpc_client_command_rsp_event_data;

AZ_NODISCARD az_result az_mqtt5_rpc_client_invoke_command(
    az_mqtt5_rpc_client_hfsm* client,
    az_mqtt5_rpc_client_command_req_event_data* data);

AZ_NODISCARD az_result az_rpc_client_hfsm_init(
    az_mqtt5_rpc_client_hfsm* client,
    az_mqtt5_rpc_client* rpc_client,
    az_mqtt5_connection* connection,
    az_mqtt5_property_bag property_bag,
    az_span client_id,
    az_span model_id,
    az_span executor_client_id,
    az_span command_name,
    az_span response_topic_buffer,
    az_span request_topic_buffer,
    az_mqtt5_rpc_client_options* options);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_RPC_CLIENT_H
