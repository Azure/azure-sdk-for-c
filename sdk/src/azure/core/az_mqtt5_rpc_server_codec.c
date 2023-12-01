// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5_rpc.h>
#include <azure/core/az_mqtt5_rpc_server_codec.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_mqtt5_topic_parser_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/iot/az_iot_common.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

static const az_span _az_mqtt5_rpc_any_executor_id
    = AZ_SPAN_LITERAL_FROM_STR(_az_MQTT5_TOPIC_PARSER_ANY_EXECUTOR_ID);

AZ_NODISCARD az_mqtt5_rpc_server_codec_options az_mqtt5_rpc_server_codec_options_default()
{
  return (az_mqtt5_rpc_server_codec_options){ .service_group_id = AZ_SPAN_EMPTY,
                                              .subscription_topic_format = AZ_SPAN_LITERAL_FROM_STR(
                                                  AZ_MQTT5_RPC_DEFAULT_REQUEST_TOPIC_FORMAT) };
}

AZ_NODISCARD az_result az_mqtt5_rpc_server_codec_get_subscribe_topic(
    az_mqtt5_rpc_server_codec* server,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  _az_PRECONDITION_NOT_NULL(server);
  _az_PRECONDITION_NOT_NULL(mqtt_topic);
  _az_PRECONDITION_RANGE(1, mqtt_topic_size, INT32_MAX);

  az_result ret = AZ_OK;

  az_span mqtt_topic_span = az_span_create((uint8_t*)mqtt_topic, (int32_t)mqtt_topic_size);
  uint32_t required_length = 0;

  ret = _az_mqtt5_topic_parser_replace_tokens_in_format(
      mqtt_topic_span,
      server->_internal.options.subscription_topic_format,
      server->_internal.options.service_group_id,
      AZ_SPAN_EMPTY,
      server->_internal.model_id,
      !az_span_is_content_equal(server->_internal.options.service_group_id, AZ_SPAN_EMPTY)
          ? _az_mqtt5_rpc_any_executor_id
          : server->_internal.client_id,
      AZ_SPAN_EMPTY,
      AZ_SPAN_FROM_STR(_az_MQTT5_TOPIC_PARSER_SINGLE_LEVEL_WILDCARD_TOKEN),
      &required_length);

  if (out_mqtt_topic_length)
  {
    *out_mqtt_topic_length = (size_t)required_length;
  }

  return ret;
}

AZ_NODISCARD az_result az_mqtt5_rpc_server_codec_parse_received_topic(
    az_mqtt5_rpc_server_codec* server,
    az_span received_topic,
    az_mqtt5_rpc_server_codec_request* out_request)
{
  _az_PRECONDITION_NOT_NULL(server);
  _az_PRECONDITION_VALID_SPAN(received_topic, 1, false);
  _az_PRECONDITION_NOT_NULL(out_request);

  return _az_mqtt5_topic_parser_extract_tokens_from_topic(
      server->_internal.options.subscription_topic_format,
      received_topic,
      AZ_SPAN_EMPTY,
      server->_internal.model_id,
      az_span_is_content_equal(server->_internal.options.service_group_id, AZ_SPAN_EMPTY)
          ? server->_internal.client_id
          : _az_mqtt5_rpc_any_executor_id,
      AZ_SPAN_EMPTY,
      NULL,
      &out_request->service_id,
      &out_request->executor_id,
      NULL,
      &out_request->command_name);
}

AZ_NODISCARD az_result az_mqtt5_rpc_server_codec_init(
    az_mqtt5_rpc_server_codec* server,
    az_span model_id,
    az_span client_id,
    az_mqtt5_rpc_server_codec_options* options)
{
  _az_PRECONDITION_NOT_NULL(server);

  if (options == NULL)
  {
    server->_internal.options = az_mqtt5_rpc_server_codec_options_default();
  }
  else if (_az_mqtt5_topic_parser_valid_topic_format(options->subscription_topic_format))
  {
    server->_internal.options = *options;
  }
  else
  {
    return AZ_ERROR_ARG;
  }

  server->_internal.client_id = client_id;
  server->_internal.model_id = model_id;

  return AZ_OK;
}
