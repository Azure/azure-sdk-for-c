// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5_rpc_server.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_mqtt5_rpc_server_codec_options az_mqtt5_rpc_server_codec_options_default()
{
  return (az_mqtt5_rpc_server_codec_options){ .subscribe_timeout_in_seconds
                                              = AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS,
                                              .subscription_topic_format = AZ_SPAN_EMPTY };
}

AZ_NODISCARD az_result az_mqtt5_rpc_server_codec_get_subscription_topic(
    az_mqtt5_rpc_server_codec* client,
    az_span out_subscription_topic)
{
  return az_rpc_get_topic_from_format(
      client->_internal.options.subscription_topic_format,
      client->_internal.model_id,
      client->_internal.client_id,
      AZ_SPAN_EMPTY,
      _az_span_is_valid(client->_internal.command_name, 1, 0) ? client->_internal.command_name
                                                              : AZ_SPAN_FROM_STR("+"),
      out_subscription_topic,
      NULL);
}

AZ_NODISCARD az_result az_mqtt5_rpc_server_codec_init(
    az_mqtt5_rpc_server_codec* client,
    az_span model_id,
    az_span client_id,
    az_span command_name,
    az_span subscription_topic,
    az_mqtt5_rpc_server_codec_options* options)
{
  _az_PRECONDITION_NOT_NULL(client);
  if (options != NULL && options->subscribe_timeout_in_seconds == 0)
  {
    return AZ_ERROR_ARG;
  }

  client->_internal.options
      = options == NULL ? az_mqtt5_rpc_server_codec_options_default() : *options;

  client->_internal.client_id = client_id;
  client->_internal.model_id = model_id;
  client->_internal.command_name = command_name;

  _az_RETURN_IF_FAILED(
      az_mqtt5_rpc_server_codec_get_subscription_topic(client, subscription_topic));

  client->_internal.subscription_topic = subscription_topic;

  return AZ_OK;
}
