// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_event_policy.h>
#include <azure/core/az_mqtt5_config.h>
#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/iot/az_iot_common.h>

#include <azure/core/_az_cfg.h>
#include "azure/core/az_mqtt5_rpc_client.h"

AZ_NODISCARD bool az_mqtt5_connection_credential_swap_condition_default(
    az_mqtt5_connack_data connack_data)
{
  return (connack_data.tls_authentication_error)
      || (connack_data.connack_reason == AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR)
      || (connack_data.connack_reason == AZ_MQTT5_CONNACK_NOT_AUTHORIZED)
      || (connack_data.connack_reason == AZ_MQTT5_CONNACK_SERVER_BUSY)
      || (connack_data.connack_reason == AZ_MQTT5_CONNACK_BANNED)
      || (connack_data.connack_reason == AZ_MQTT5_CONNACK_BAD_AUTHENTICATION_METHOD);
}

AZ_NODISCARD az_mqtt5_connection_options az_mqtt5_connection_options_default()
{
  return (az_mqtt5_connection_options){
    .retry_delay_function = az_iot_calculate_retry_delay,
    .credential_swap_condition = az_mqtt5_connection_credential_swap_condition_default,
    .hostname = AZ_SPAN_EMPTY,
    .port = AZ_MQTT5_DEFAULT_CONNECT_PORT,
    .client_certificates_count = 0,
    .disable_sdk_connection_management = false,
    .client_id_buffer = AZ_SPAN_EMPTY,
    .username_buffer = AZ_SPAN_EMPTY,
    .password_buffer = AZ_SPAN_EMPTY,
    .client_certificates = NULL,
  };
}

static az_result _az_mqtt5_connection_event_relay_process_outbound_event(
    az_event_policy* policy,
    az_event const event)
{
  _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event(policy, event));

  return AZ_OK;
}

static az_result _az_mqtt5_connection_event_relay_process_inbound_event(
    az_event_policy* policy,
    az_event const event)
{
  _az_mqtt5_connection_event_relay* relay = (_az_mqtt5_connection_event_relay*)policy;

  if (relay->_internal.connection->_internal.options.disable_sdk_connection_management)
  {
    if (relay->_internal.event_callback != NULL)
    {
      _az_RETURN_IF_FAILED(relay->_internal.event_callback(
          relay->_internal.connection, event, relay->_internal.event_callback_context));
    }
    return AZ_OK;
  }

  switch (event.type)
  {
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
    case AZ_MQTT5_EVENT_PUBACK_RSP:
    case AZ_MQTT5_EVENT_SUBACK_RSP:
    case AZ_MQTT5_EVENT_UNSUBACK_RSP:
    case AZ_MQTT5_EVENT_PUB_RECV_IND:
    case AZ_MQTT5_EVENT_REQUEST_INIT:
    case AZ_MQTT5_EVENT_RPC_CLIENT_REMOVE_REQ:
    case AZ_MQTT5_EVENT_REQUEST_COMPLETE:
    case AZ_MQTT5_EVENT_REQUEST_FAULTED:
    case AZ_HFSM_EVENT_TIMEOUT:
      break;
    default:
      if (relay->_internal.event_callback != NULL)
      {
        _az_RETURN_IF_FAILED(relay->_internal.event_callback(
            relay->_internal.connection, event, relay->_internal.event_callback_context));
      }
      break;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_connection_init(
    az_mqtt5_connection* client,
    az_context* context,
    az_mqtt5* mqtt_client,
    az_mqtt5_connection_callback event_callback,
    az_mqtt5_connection_options* options,
    void* event_callback_context)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(mqtt_client);
  _az_PRECONDITION_NOT_NULL(event_callback);

  client->_internal.options = options == NULL ? az_mqtt5_connection_options_default() : *options;

  _az_PRECONDITION_RANGE(-1, AZ_MQTT5_CONNECTION_MAX_CONNECT_ATTEMPTS, INT16_MAX - 1);
  _az_PRECONDITION_RANGE(0, AZ_MQTT5_CONNECTION_MIN_RETRY_DELAY_MSEC, INT32_MAX - 1);
  _az_PRECONDITION_RANGE(0, AZ_MQTT5_CONNECTION_MAX_RETRY_DELAY_MSEC, INT32_MAX - 1);
  _az_PRECONDITION_RANGE(0, AZ_MQTT5_CONNECTION_MAX_RANDOM_JITTER_MSEC, INT32_MAX - 1);
  _az_PRECONDITION_VALID_SPAN(client->_internal.options.hostname, 1, false);
  _az_PRECONDITION_VALID_SPAN(client->_internal.options.client_id_buffer, 1, false);
  _az_PRECONDITION_VALID_SPAN(client->_internal.options.username_buffer, 1, false);
  _az_PRECONDITION_VALID_SPAN(client->_internal.options.password_buffer, 0, true);
  if (client->_internal.options.client_certificates_count > 0)
  {
    _az_PRECONDITION_NOT_NULL(client->_internal.options.client_certificates);
  }
#ifndef AZ_NO_PRECONDITION_CHECKING
  for (int i = 0; i < client->_internal.options.client_certificates_count; i++)
  {
    _az_PRECONDITION_VALID_SPAN(client->_internal.options.client_certificates[i].cert, 1, false);
    _az_PRECONDITION_VALID_SPAN(client->_internal.options.client_certificates[i].key, 1, false);
  }
#endif // AZ_NO_PRECONDITION_CHECKING

  client->_internal.client_certificate_index = 0;
  client->_internal.reconnect_counter = 0;
  client->_internal.connect_time_msec = 0;
  client->_internal.connect_start_time_msec = 0;

  if (!client->_internal.options.disable_sdk_connection_management)
  {
    // The pipeline contains the connection_policy.
    // event_relay_policy --> policy_collection --> connection_policy --> az_mqtt5_policy
    //    outbound                                                            inbound

    _az_RETURN_IF_FAILED(_az_mqtt5_policy_init(
        &client->_internal.mqtt5_policy,
        mqtt_client,
        context,
        NULL,
        (az_event_policy*)&client->_internal.connection_policy));

    _az_RETURN_IF_FAILED(_az_mqtt5_connection_policy_init(
        (_az_hfsm*)client,
        (az_event_policy*)&client->_internal.mqtt5_policy,
        (az_event_policy*)&client->_internal.policy_collection));

    _az_RETURN_IF_FAILED(_az_event_policy_collection_init(
        &client->_internal.policy_collection,
        (az_event_policy*)client,
        (az_event_policy*)&client->_internal.event_relay_policy));
  }
  else
  {
    // The pipeline does not contain the connection_policy.
    // event_relay_policy --> subclients_policy --> az_mqtt5_policy
    //    outbound                                      inbound
    _az_RETURN_IF_FAILED(_az_mqtt5_policy_init(
        &client->_internal.mqtt5_policy,
        mqtt_client,
        context,
        NULL,
        (az_event_policy*)&client->_internal.policy_collection));

    // Unused. Initialize to NULL.
    client->_internal.connection_policy = (_az_hfsm){ 0 };

    _az_RETURN_IF_FAILED(_az_event_policy_collection_init(
        &client->_internal.policy_collection,
        (az_event_policy*)&client->_internal.mqtt5_policy,
        (az_event_policy*)&client->_internal.event_relay_policy));
  }

  client->_internal.event_relay_policy._internal.connection = client;
  client->_internal.event_relay_policy._internal.relay_policy.outbound_policy
      = (az_event_policy*)&client->_internal.policy_collection;
  client->_internal.event_relay_policy._internal.relay_policy.inbound_policy = NULL;
  client->_internal.event_relay_policy._internal.relay_policy.outbound_handler
      = _az_mqtt5_connection_event_relay_process_outbound_event;
  client->_internal.event_relay_policy._internal.relay_policy.inbound_handler
      = _az_mqtt5_connection_event_relay_process_inbound_event;
  client->_internal.event_relay_policy._internal.event_callback = event_callback;
  client->_internal.event_relay_policy._internal.event_callback_context = event_callback_context;

  _az_RETURN_IF_FAILED(_az_event_pipeline_init(
      &client->_internal.event_pipeline,
      (az_event_policy*)&client->_internal.event_relay_policy,
      (az_event_policy*)&client->_internal.mqtt5_policy));

  // Attach the az_mqtt5 client to this pipeline.
  mqtt_client->_internal.platform_mqtt5.pipeline = &client->_internal.event_pipeline;

  return AZ_OK;
}
