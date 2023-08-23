// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5_rpc_client.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_mqtt5_rpc_client_options az_mqtt5_rpc_client_options_default()
{
  return (az_mqtt5_rpc_client_options){ .subscribe_qos = AZ_MQTT5_RPC_QOS,
                                        .request_qos = AZ_MQTT5_RPC_QOS,
                                        .subscribe_timeout_in_seconds
                                        = AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS };
}

// "vehicles/dtmi:rpc:samples:vehicle;1/commands/vehicle03/unlock/__for_mobile-app"
AZ_NODISCARD az_result az_rpc_client_get_response_topic(az_mqtt5_rpc_client* client, az_span* out_response_topic)
{
// #ifndef AZ_NO_PRECONDITION_CHECKING
  _az_PRECONDITION_VALID_SPAN(client->model_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(client->client_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(client->command_name, 1, false);
  _az_PRECONDITION_VALID_SPAN(client->executor_client_id, 1, false);
  int32_t response_topic_min_length = az_span_size(client->model_id) + az_span_size(client->client_id)
      + az_span_size(client->command_name) + az_span_size(client->executor_client_id) + 27;
  _az_PRECONDITION_VALID_SPAN(*out_response_topic, response_topic_min_length, true);
// #endif

  az_span temp_span = *out_response_topic;
  temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR("vehicles/"));
  temp_span = az_span_copy(temp_span, client->model_id);
  temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR("/commands/"));
  temp_span = az_span_copy(temp_span, client->executor_client_id);
  temp_span = az_span_copy_u8(temp_span, '/');
  temp_span = az_span_copy(temp_span, client->command_name);
  temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR("/__for_"));
  temp_span = az_span_copy(temp_span, client->client_id);
  temp_span = az_span_copy_u8(temp_span, '\0');

  *out_response_topic
      = az_span_slice(*out_response_topic, 0, response_topic_min_length);

  return AZ_OK;
}

// "vehicles/dtmi:rpc:samples:vehicle;1/commands/vehicle03/unlock"
AZ_NODISCARD az_result az_rpc_client_get_request_topic(az_mqtt5_rpc_client* client, az_span out_request_topic)
{
#ifndef AZ_NO_PRECONDITION_CHECKING
  _az_PRECONDITION_VALID_SPAN(client->model_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(client->executor_client_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(client->command_name, 1, false);
  int32_t request_topic_min_length = az_span_size(client->model_id) + az_span_size(client->executor_client_id)
      + az_span_size(client->command_name) + 23;
  _az_PRECONDITION_VALID_SPAN(out_request_topic, request_topic_min_length, true);
#endif

  az_span temp_span = out_request_topic;
  temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR("vehicles/"));
  temp_span = az_span_copy(temp_span, client->model_id);
  temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR("/commands/"));
  temp_span = az_span_copy(temp_span, client->executor_client_id);
  temp_span = az_span_copy_u8(temp_span, '/');
  temp_span = az_span_copy(temp_span, client->command_name);
  temp_span = az_span_copy_u8(temp_span, '\0');

  return AZ_OK;
}

AZ_NODISCARD az_result az_rpc_client_init(
    az_mqtt5_rpc_client* client,
    az_span client_id,
    az_span model_id,
    az_span executor_client_id,
    az_span command_name,
    az_span response_topic_buffer,
    az_span request_topic_buffer,
    az_mqtt5_rpc_client_options* options)
{
  _az_PRECONDITION_NOT_NULL(client);
  client->options = options == NULL ? az_mqtt5_rpc_client_options_default() : *options;
  
  // For now, we only support QoS 1
  if (client->options.subscribe_qos != 1 || client->options.request_qos != 1)
  {
    return AZ_ERROR_ARG;
  }

  if (client->options.subscribe_timeout_in_seconds <= 0)
  {
    return AZ_ERROR_ARG;
  }

  _az_PRECONDITION_VALID_SPAN(client_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(model_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(executor_client_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(command_name, 1, false);

  client->client_id = client_id;
  client->model_id = model_id;
  client->command_name = command_name;
  client->executor_client_id = executor_client_id;

  _az_RETURN_IF_FAILED(az_rpc_client_get_response_topic(client, &response_topic_buffer));
  client->response_topic = response_topic_buffer;

  _az_RETURN_IF_FAILED(az_rpc_client_get_request_topic(client, request_topic_buffer));
  client->request_topic = request_topic_buffer;

  return AZ_OK;
}