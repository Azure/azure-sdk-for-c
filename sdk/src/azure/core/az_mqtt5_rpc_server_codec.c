// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5_rpc.h>
#include <azure/core/az_mqtt5_rpc_server_codec.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/iot/az_iot_common.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

static const az_span az_mqtt5_rpc_service_group_id_key
    = AZ_SPAN_LITERAL_FROM_STR(AZ_MQTT5_RPC_SERVICE_GROUP_ID_KEY);
static const az_span az_mqtt5_rpc_service_id_key
    = AZ_SPAN_LITERAL_FROM_STR(AZ_MQTT5_RPC_SERVICE_ID_KEY);
static const az_span az_mqtt5_rpc_executor_id_key
    = AZ_SPAN_LITERAL_FROM_STR(AZ_MQTT5_RPC_EXECUTOR_ID_KEY);
static const az_span az_mqtt5_rpc_command_id_key
    = AZ_SPAN_LITERAL_FROM_STR(AZ_MQTT5_RPC_COMMAND_ID_KEY);
static const az_span az_mqtt5_rpc_any_executor_id
    = AZ_SPAN_LITERAL_FROM_STR(AZ_MQTT5_RPC_ANY_EXECUTOR_ID);

AZ_NODISCARD az_mqtt5_rpc_server_codec_options az_mqtt5_rpc_server_codec_options_default()
{
  return (az_mqtt5_rpc_server_codec_options){ .service_group_id = AZ_SPAN_EMPTY,
                                              .subscription_topic_format = AZ_SPAN_LITERAL_FROM_STR(
                                                  AZ_MQTT5_RPC_DEFAULT_REQUEST_TOPIC_FORMAT) };
}

AZ_NODISCARD az_result az_mqtt5_rpc_server_codec_get_subscribe_topic(
    az_mqtt5_rpc_server_codec* server,
    az_span service_group_id,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  _az_PRECONDITION_NOT_NULL(server);
  _az_PRECONDITION_NOT_NULL(mqtt_topic);
  _az_PRECONDITION(mqtt_topic_size > 0);

  az_span topic_format = server->_internal.options.subscription_topic_format;
  az_span mqtt_topic_span = az_span_create((uint8_t*)mqtt_topic, (int32_t)mqtt_topic_size);
  int32_t model_id_index = az_span_find(topic_format, az_mqtt5_rpc_service_id_key);
  int32_t executor_id_index = az_span_find(topic_format, az_mqtt5_rpc_executor_id_key);
  int32_t command_id_index = az_span_find(topic_format, az_mqtt5_rpc_command_id_key);

  int32_t required_length = az_span_size(server->_internal.options.subscription_topic_format)
      + (int32_t)sizeof((uint8_t)'\0');

  required_length
      += (az_span_is_content_equal(service_group_id, AZ_SPAN_EMPTY)
              ? 0
              : az_span_size(az_mqtt5_rpc_service_group_id_key) + az_span_size(service_group_id)
                  + 1);
  required_length += (model_id_index == -1)
      ? 0
      : az_span_size(server->_internal.model_id) - az_span_size(az_mqtt5_rpc_service_id_key);
  if (executor_id_index != -1)
  {
    required_length
        += (!az_span_is_content_equal(service_group_id, AZ_SPAN_EMPTY)
                ? az_span_size(az_mqtt5_rpc_any_executor_id)
                : az_span_size(server->_internal.client_id));
    required_length -= az_span_size(az_mqtt5_rpc_executor_id_key);
  }
  required_length += (command_id_index == -1)
      ? 0
      : (int32_t)sizeof((uint8_t)'+') - az_span_size(az_mqtt5_rpc_command_id_key);

  _az_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_topic_span, required_length);

  az_span remainder = mqtt_topic_span;
  if (!az_span_is_content_equal(service_group_id, AZ_SPAN_EMPTY))
  {
    remainder = az_span_copy(mqtt_topic_span, az_mqtt5_rpc_service_group_id_key);
    remainder = az_span_copy(remainder, service_group_id);
    remainder = az_span_copy_u8(remainder, '/');
  }

  for (int32_t i = 0; i < az_span_size(topic_format); i++)
  {
    uint8_t c = az_span_ptr(topic_format)[i];

    if (c == '{')
    {
      if (i == model_id_index)
      {
        remainder = az_span_copy(remainder, server->_internal.model_id);
        i += az_span_size(az_mqtt5_rpc_service_id_key) - 1;
      }
      else if (i == executor_id_index)
      {
        remainder = az_span_copy(
            remainder,
            !az_span_is_content_equal(service_group_id, AZ_SPAN_EMPTY)
                ? az_mqtt5_rpc_any_executor_id
                : server->_internal.client_id);
        i += az_span_size(az_mqtt5_rpc_executor_id_key) - 1;
      }
      else if (i == command_id_index)
      {
        remainder = az_span_copy_u8(remainder, '+');
        i += az_span_size(az_mqtt5_rpc_command_id_key) - 1;
      }
    }
    else
    {
      remainder = az_span_copy_u8(remainder, c);
    }
  }

  az_span_copy_u8(remainder, '\0');

  if (out_mqtt_topic_length)
  {
    *out_mqtt_topic_length = (size_t)required_length;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_rpc_server_codec_parse_received_topic(
    az_mqtt5_rpc_server_codec* server,
    az_span received_topic,
    az_mqtt5_rpc_server_codec_request* out_request)
{
  _az_PRECONDITION_NOT_NULL(server);
  _az_PRECONDITION_VALID_SPAN(received_topic, 1, false);
  _az_PRECONDITION_NOT_NULL(out_request);

  az_span topic_format = server->_internal.options.subscription_topic_format;
  az_span executor_id
      = (az_span_is_content_equal(server->_internal.options.service_group_id, AZ_SPAN_EMPTY)
             ? server->_internal.client_id
             : az_mqtt5_rpc_any_executor_id);

  int32_t model_id_index = az_span_find(topic_format, az_mqtt5_rpc_service_id_key);
  int32_t executor_id_index = az_span_find(topic_format, az_mqtt5_rpc_executor_id_key);
  int32_t command_id_index = az_span_find(topic_format, az_mqtt5_rpc_command_id_key);

  int32_t format_idx = 0;
  uint8_t format_char;
  for (int32_t i = 0; i < az_span_size(received_topic); i++)
  {
    uint8_t c = az_span_ptr(received_topic)[i];
    if (format_idx < az_span_size(topic_format))
    {
      format_char = az_span_ptr(topic_format)[format_idx];
      if (format_char == '{')
      {
        if (format_idx == model_id_index
            && (i + az_span_size(server->_internal.model_id) - 1) < az_span_size(received_topic))
        {
          az_span extracted_model_id
              = az_span_slice(received_topic, i, i + az_span_size(server->_internal.model_id));
          if (az_span_is_content_equal(extracted_model_id, server->_internal.model_id))
          {
            i += az_span_size(server->_internal.model_id) - 1;
            format_idx += az_span_size(az_mqtt5_rpc_service_id_key) - 1;
            out_request->service_id = server->_internal.model_id;
          }
          else
          {
            return AZ_ERROR_IOT_TOPIC_NO_MATCH;
          }
        }
        else if (
            format_idx == executor_id_index
            && (i + az_span_size(executor_id) - 1) < az_span_size(received_topic))
        {
          az_span extracted_client_id
              = az_span_slice(received_topic, i, i + az_span_size(executor_id));
          if (az_span_is_content_equal(extracted_client_id, executor_id))
          {
            i += az_span_size(executor_id) - 1;
            format_idx += az_span_size(az_mqtt5_rpc_executor_id_key) - 1;
            out_request->executor_id = executor_id;
          }
          else
          {
            return AZ_ERROR_IOT_TOPIC_NO_MATCH;
          }
        }
        else if (format_idx == command_id_index)
        {
          az_span remaining_topic = az_span_slice_to_end(received_topic, i);
          int32_t backslash_idx = az_span_find(remaining_topic, AZ_SPAN_FROM_STR("/"));
          if (backslash_idx == -1)
          {
            out_request->command_name = remaining_topic;
          }
          else
          {
            out_request->command_name = az_span_slice(remaining_topic, 0, backslash_idx);
          }
          i += az_span_size(out_request->command_name) - 1;
          format_idx += az_span_size(az_mqtt5_rpc_command_id_key) - 1;
        }
        else
        {
          return AZ_ERROR_IOT_TOPIC_NO_MATCH;
        }
      }
      else if (c != format_char)
      {
        return AZ_ERROR_IOT_TOPIC_NO_MATCH;
      }
    }
    else
    {
      return AZ_ERROR_IOT_TOPIC_NO_MATCH;
    }
    format_idx++;
  }

  if (format_idx < az_span_size(topic_format))
  {
    return AZ_ERROR_IOT_TOPIC_NO_MATCH;
  }

  return AZ_OK;
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
  else if (_az_mqtt5_rpc_valid_topic_format(options->subscription_topic_format))
  {
    server->_internal.options = *options;
  }
  else
  {
    return AZ_ERROR_ARG;
  }

  server->_internal.options
      = options == NULL ? az_mqtt5_rpc_server_codec_options_default() : *options;

  server->_internal.client_id = client_id;
  server->_internal.model_id = model_id;

  return AZ_OK;
}
