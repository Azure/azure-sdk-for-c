// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5_rpc_client.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_mqtt5_rpc_client_options az_mqtt5_rpc_client_options_default()
{
  return (az_mqtt5_rpc_client_options){ .subscribe_timeout_in_seconds
                                        = AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS,
                                        .publish_timeout_in_seconds
                                        = AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS,
                                        .subscription_topic_format = AZ_SPAN_FROM_STR("vehicles/{serviceId}/commands/{executorId}/{name}/__for_{invokerId}\0"),
                                        .request_topic_format = AZ_SPAN_FROM_STR("vehicles/{serviceId}/commands/{executorId}/{name}\0") };
}

AZ_NODISCARD az_result az_rpc_client_get_subscription_topic(
    az_mqtt5_rpc_client* client,
    az_span out_subscription_topic,
    int32_t* out_topic_length)
{
  return az_rpc_get_topic_from_format(
    client->_internal.model_id,
    AZ_SPAN_FROM_STR("+"),
    client->_internal.client_id,
    client->_internal.command_name,
    client->_internal.options.subscription_topic_format,
    out_subscription_topic,
    out_topic_length);
}

// Replaces + in subscription topic with server client id
AZ_NODISCARD az_result az_rpc_client_get_response_topic(
    az_mqtt5_rpc_client* client,
    az_span server_client_id,
    az_span out_response_topic)
{
#ifndef AZ_NO_PRECONDITION_CHECKING
  _az_PRECONDITION_VALID_SPAN(client->_internal.subscription_topic, 1, false);
  _az_PRECONDITION_VALID_SPAN(server_client_id, 1, false);
  int32_t response_topic_min_length
      = az_span_size(client->_internal.subscription_topic) + az_span_size(server_client_id) - 1;
  _az_PRECONDITION_VALID_SPAN(out_response_topic, response_topic_min_length, true);
#endif

  az_span_fill(out_response_topic, ' ');

  int32_t index = az_span_find(client->_internal.subscription_topic, AZ_SPAN_FROM_STR("+"));
  if (index > 0)
  {
    out_response_topic = az_span_copy(
        out_response_topic, az_span_slice(client->_internal.subscription_topic, 0, index));
    out_response_topic = az_span_copy(out_response_topic, server_client_id);
    out_response_topic = az_span_copy(
        out_response_topic, az_span_slice_to_end(client->_internal.subscription_topic, index + 1));
  }
  else
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_rpc_client_get_request_topic(
    az_mqtt5_rpc_client* client,
    az_span server_client_id,
    az_span out_request_topic)
{
  return az_rpc_get_topic_from_format(
    client->_internal.model_id,
    server_client_id,
    client->_internal.client_id,
    client->_internal.command_name,
    client->_internal.options.request_topic_format,
    out_request_topic,
    NULL);
}

AZ_NODISCARD az_result az_rpc_client_init(
    az_mqtt5_rpc_client* client,
    az_span client_id,
    az_span model_id,
    az_span command_name,
    az_span response_topic_buffer,
    az_span request_topic_buffer,
    az_span subscribe_topic_buffer,
    az_mqtt5_rpc_client_options* options)
{
  _az_PRECONDITION_NOT_NULL(client);
  client->_internal.options = options == NULL ? az_mqtt5_rpc_client_options_default() : *options;

  if (client->_internal.options.subscribe_timeout_in_seconds <= 0
      || client->_internal.options.publish_timeout_in_seconds <= 0)
  {
    return AZ_ERROR_ARG;
  }

  _az_PRECONDITION_VALID_SPAN(client_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(model_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(command_name, 1, false);

  client->_internal.client_id = client_id;
  client->_internal.model_id = model_id;
  client->_internal.command_name = command_name;
  client->_internal.response_topic_buffer = response_topic_buffer;
  client->_internal.request_topic_buffer = request_topic_buffer;

  int32_t topic_length;

  _az_RETURN_IF_FAILED(
      az_rpc_client_get_subscription_topic(client, subscribe_topic_buffer, &topic_length));
  client->_internal.subscription_topic = az_span_slice(subscribe_topic_buffer, 0, topic_length);

  return AZ_OK;
}
