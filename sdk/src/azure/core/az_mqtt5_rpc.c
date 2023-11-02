// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5_rpc.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <stdio.h>

#if defined(TRANSPORT_MOSQUITTO)
#include <mosquitto.h>
#endif

#include <azure/core/_az_cfg.h>

static const az_span service_group_id_key
    = AZ_SPAN_LITERAL_FROM_STR(AZ_MQTT5_RPC_SERVICE_GROUP_ID_KEY);
static const az_span model_id_key = AZ_SPAN_LITERAL_FROM_STR(AZ_MQTT5_RPC_SERVICE_ID_KEY);
static const az_span command_name_key = AZ_SPAN_LITERAL_FROM_STR(AZ_MQTT5_RPC_COMMAND_ID_KEY);
static const az_span executor_client_id_key
    = AZ_SPAN_LITERAL_FROM_STR(AZ_MQTT5_RPC_EXECUTOR_ID_KEY);
static const az_span invoker_client_id_key = AZ_SPAN_LITERAL_FROM_STR(AZ_MQTT5_RPC_CLIENT_ID_KEY);
static const az_span token_end_character = AZ_SPAN_LITERAL_FROM_STR("}");

static const uint32_t az_mqtt5_rpc_client_id_hash = 3426466449;
static const uint32_t az_mqtt5_rpc_service_id_hash = 4175641829;
static const uint32_t az_mqtt5_rpc_executor_id_hash = 3913329219;
static const uint32_t az_mqtt5_rpc_command_id_hash = 2624200456;

AZ_INLINE uint32_t _az_mqtt5_rpc_calculate_hash(az_span token)
{
  uint32_t hash = 5831;
  for (int32_t i = 0; i < az_span_size(token); i++)
  {
    hash = ((hash << 5) + hash) + az_span_ptr(token)[i];
  }
  return hash;
}

AZ_INLINE az_span _az_mqtt5_rpc_extract_token(az_span format)
{
  int32_t pos = az_span_find(format, token_end_character);
  if (pos == -1)
  {
    return AZ_SPAN_EMPTY;
  }
  return az_span_slice(format, 0, pos);
}

AZ_INLINE az_result _az_mqtt5_rpc_verify_buffer_length_and_copy(
    az_span value,
    az_span* remainder,
    uint32_t* required_length,
    uint32_t buffer_length)
{
  if (!_az_span_is_valid(value, 1, false))
  {
    return AZ_ERROR_ARG;
  }
  *required_length += (uint32_t)az_span_size(value) - 1;
  if (*required_length > (uint32_t)buffer_length)
  {
    return AZ_ERROR_NOT_ENOUGH_SPACE;
  }
  *remainder = az_span_copy(*remainder, value);

  return AZ_OK;
}

AZ_NODISCARD az_result _az_mqtt5_rpc_replace_tokens_in_format(
    az_span mqtt_topic_span,
    az_span topic_format,
    az_span service_group_id,
    az_span client_id,
    az_span service_id,
    az_span executor_id,
    az_span command_id,
    uint32_t* required_length)
{
  az_result ret = AZ_OK;
  *required_length += (int32_t)sizeof((uint8_t)'\0');
  uint32_t buffer_length = (uint32_t)az_span_size(mqtt_topic_span);
  az_span remainder = mqtt_topic_span;

  if (!az_span_is_content_equal(service_group_id, AZ_SPAN_EMPTY))
  {
    *required_length += (uint32_t)az_span_size(service_group_id_key)
        + (uint32_t)az_span_size(service_group_id) + 1;
    _az_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_topic_span, (int32_t)*required_length);
    remainder = az_span_copy(remainder, service_group_id_key);
    remainder = az_span_copy(remainder, service_group_id);
    remainder = az_span_copy_u8(remainder, '/');
  }

  for (int32_t i = 0; i < az_span_size(topic_format); i++)
  {
    uint8_t c = az_span_ptr(topic_format)[i];
    _az_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_topic_span, (int32_t)(++*required_length));

    if (c == '{')
    {
      if (i + 1 >= az_span_size(topic_format))
      {
        return AZ_ERROR_ARG;
      }
      az_span token = _az_mqtt5_rpc_extract_token(az_span_slice_to_end(topic_format, i + 1));
      if (az_span_is_content_equal(token, AZ_SPAN_EMPTY))
      {
        return AZ_ERROR_ARG;
      }
      uint32_t curr_hash = _az_mqtt5_rpc_calculate_hash(token);
      if (curr_hash == az_mqtt5_rpc_client_id_hash)
      {
        ret = _az_mqtt5_rpc_verify_buffer_length_and_copy(
            client_id, &remainder, required_length, buffer_length);
        i += az_span_size(invoker_client_id_key) - 1;
      }
      else if (curr_hash == az_mqtt5_rpc_service_id_hash)
      {
        ret = _az_mqtt5_rpc_verify_buffer_length_and_copy(
            service_id, &remainder, required_length, buffer_length);
        i += az_span_size(model_id_key) - 1;
      }
      else if (curr_hash == az_mqtt5_rpc_executor_id_hash)
      {
        ret = _az_mqtt5_rpc_verify_buffer_length_and_copy(
            executor_id, &remainder, required_length, buffer_length);
        i += az_span_size(executor_client_id_key) - 1;
      }
      else if (curr_hash == az_mqtt5_rpc_command_id_hash)
      {
        ret = _az_mqtt5_rpc_verify_buffer_length_and_copy(
            command_id, &remainder, required_length, buffer_length);
        i += az_span_size(command_name_key) - 1;
      }
      else
      {
        return AZ_ERROR_ARG;
      }
    }
    else
    {
      remainder = az_span_copy_u8(remainder, c);
    }
  }

  az_span_copy_u8(remainder, '\0');

  return ret;
}

/**
 * @brief Helper function to obtain the next level of a topic.
 *
 * @param topic Topic to get the next level of.
 * @return An #az_span value containing the next level of the topic including the backslash.
 */
AZ_NODISCARD AZ_INLINE az_span _get_next_topic_level(az_span topic)
{
  int32_t pos = az_span_find(topic, AZ_SPAN_FROM_STR("/"));
  if (pos == -1)
  {
    int32_t pos_null_char = az_span_find(topic, AZ_SPAN_FROM_STR("\0"));
    if (pos_null_char == -1)
    {
      return topic;
    }
    return az_span_slice(topic, 0, pos_null_char);
  }

  return az_span_slice(topic, 0, pos + 1);
}

/**
 * @brief Helper function to check if a topic has a backslash at the end of it.
 *
 * @param topic Topic to check.
 * @return True if the topic has a backslash at the end of it, false otherwise.
 */
AZ_NODISCARD AZ_INLINE bool _has_backslash_in_topic(az_span topic)
{
  if (az_span_size(topic) == 0)
  {
    return false;
  }
  return az_span_find(topic, AZ_SPAN_FROM_STR("/")) == az_span_size(topic) - 1;
}

AZ_NODISCARD bool az_mqtt5_rpc_status_failed(az_mqtt5_rpc_status status)
{
  return (status < 200 || status >= 300);
}

AZ_INLINE AZ_NODISCARD bool _az_mqtt5_is_key_surrounded_by_slash(
    int32_t idx,
    int32_t key_size,
    az_span topic_format)
{
  return (
      (idx == 0 && (az_span_size(topic_format) == idx + key_size))
      || (idx == 0 && az_span_ptr(topic_format)[idx + key_size] == '/')
      || (idx > 0 && az_span_ptr(topic_format)[idx - 1] == '/'
          && (az_span_size(topic_format) == idx + key_size))
      || (idx > 0 && az_span_ptr(topic_format)[idx - 1] == '/'
          && az_span_ptr(topic_format)[idx + key_size] == '/'));
}

AZ_NODISCARD bool _az_mqtt5_rpc_valid_topic_format(az_span topic_format)
{
  int32_t service_group_id_index = az_span_find(topic_format, service_group_id_key);
  if (service_group_id_index != -1)
  {
    if (!_az_mqtt5_is_key_surrounded_by_slash(
            service_group_id_index, az_span_size(service_group_id_key), topic_format))
    {
      return false;
    }
  }

  int32_t model_id_index = az_span_find(topic_format, model_id_key);
  if (model_id_index != -1)
  {
    if (!_az_mqtt5_is_key_surrounded_by_slash(
            model_id_index, az_span_size(model_id_key), topic_format))
    {
      return false;
    }
  }

  int32_t command_name_index = az_span_find(topic_format, command_name_key);
  if (command_name_index != -1)
  {
    if (!_az_mqtt5_is_key_surrounded_by_slash(
            command_name_index, az_span_size(command_name_key), topic_format))
    {
      return false;
    }
  }

  int32_t executor_client_id_index = az_span_find(topic_format, executor_client_id_key);
  if (executor_client_id_index != -1)
  {
    if (!_az_mqtt5_is_key_surrounded_by_slash(
            executor_client_id_index, az_span_size(executor_client_id_key), topic_format))
    {
      return false;
    }
  }

  int32_t invoker_client_id_index = az_span_find(topic_format, invoker_client_id_key);
  if (invoker_client_id_index != -1)
  {
    if (!_az_mqtt5_is_key_surrounded_by_slash(
            invoker_client_id_index, az_span_size(invoker_client_id_key), topic_format))
    {
      return false;
    }
  }
  return true;
}
