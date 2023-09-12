// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5_rpc_server.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_mqtt5_rpc_server_options az_mqtt5_rpc_server_options_default()
{
  return (az_mqtt5_rpc_server_options){
    .subscribe_timeout_in_seconds = AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS,
  };
}

AZ_NODISCARD az_result
az_rpc_server_get_subscription_topic(az_mqtt5_rpc_server* client, az_span out_subscription_topic)
{
  _az_PRECONDITION_NOT_NULL(client);
#ifndef AZ_NO_PRECONDITION_CHECKING
  _az_PRECONDITION_VALID_SPAN(client->_internal.model_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(client->_internal.client_id, 1, false);
  int32_t subscription_min_length = az_span_size(client->_internal.model_id)
      + az_span_size(client->_internal.client_id)
      + (az_span_size(client->_internal.command_name) > 0
             ? az_span_size(client->_internal.command_name)
             : 1)
      + 23;
  _az_PRECONDITION_VALID_SPAN(out_subscription_topic, subscription_min_length, true);
#endif

  // TODO: Create generic function to create topics from dtdl format
  az_span temp_span = out_subscription_topic;
  temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR("vehicles/"));
  temp_span = az_span_copy(temp_span, client->_internal.model_id);
  temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR("/commands/"));
  temp_span = az_span_copy(temp_span, client->_internal.client_id);
  temp_span = az_span_copy_u8(temp_span, '/');
  temp_span = az_span_copy(
      temp_span,
      _az_span_is_valid(client->_internal.command_name, 1, 0) ? client->_internal.command_name
                                                              : AZ_SPAN_FROM_STR("+"));
  temp_span = az_span_copy_u8(temp_span, '\0');

  return AZ_OK;
}

AZ_NODISCARD az_result az_rpc_server_init(
    az_mqtt5_rpc_server* client,
    az_span model_id,
    az_span client_id,
    az_span command_name,
    az_span subscription_topic,
    az_mqtt5_rpc_server_options* options)
{
  _az_PRECONDITION_NOT_NULL(client);
  if (options != NULL && options->subscribe_timeout_in_seconds == 0)
  {
    return AZ_ERROR_ARG;
  }

  _az_PRECONDITION_VALID_SPAN(client_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(model_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(command_name, 1, false);

  client->_internal.options = options == NULL ? az_mqtt5_rpc_server_options_default() : *options;

  client->_internal.client_id = client_id;
  client->_internal.model_id = model_id;
  client->_internal.command_name = command_name;

  _az_RETURN_IF_FAILED(az_rpc_server_get_subscription_topic(client, subscription_topic));

  client->_internal.subscription_topic = subscription_topic;

  return AZ_OK;
}
