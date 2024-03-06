// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5_rpc.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_mqtt5_topic_parser_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/iot/az_iot_common.h>
#include <stdio.h>

#if defined(TRANSPORT_MOSQUITTO)
#include <mosquitto.h>
#endif

#include <azure/core/_az_cfg.h>

static const az_span service_group_id_key
    = AZ_SPAN_LITERAL_FROM_STR(_az_MQTT5_TOPIC_PARSER_SERVICE_GROUP_ID_TOKEN);
static const az_span model_id_key = AZ_SPAN_LITERAL_FROM_STR(_az_MQTT5_TOPIC_PARSER_MODEL_ID_TOKEN);
static const az_span sender_id_key
    = AZ_SPAN_LITERAL_FROM_STR(_az_MQTT5_TOPIC_PARSER_SENDER_ID_TOKEN);
static const az_span command_name_key
    = AZ_SPAN_LITERAL_FROM_STR(_az_MQTT5_TOPIC_PARSER_COMMAND_NAME_TOKEN);
static const az_span telemetry_name_key
    = AZ_SPAN_LITERAL_FROM_STR(_az_MQTT5_TOPIC_PARSER_TELEMETRY_NAME_TOKEN);
static const az_span executor_client_id_key
    = AZ_SPAN_LITERAL_FROM_STR(_az_MQTT5_RPC_EXECUTOR_ID_TOKEN);
static const az_span invoker_client_id_key
    = AZ_SPAN_LITERAL_FROM_STR(_az_MQTT5_TOPIC_PARSER_CLIENT_ID_TOKEN);
static const az_span token_format_end_character = AZ_SPAN_LITERAL_FROM_STR("}");
static const az_span token_topic_end_character = AZ_SPAN_LITERAL_FROM_STR("/");

AZ_INLINE az_span _az_mqtt5_rpc_extract_next_token_from_format(az_span format)
{
  int32_t pos = az_span_find(format, token_format_end_character);
  if (pos == -1)
  {
    return AZ_SPAN_EMPTY;
  }
  return az_span_slice(format, 0, pos);
}

AZ_INLINE az_span _az_mqtt5_rpc_extract_next_token_from_topic(az_span topic)
{
  int32_t pos = az_span_find(topic, token_topic_end_character);
  if (pos == -1)
  {
    return az_span_slice_to_end(topic, 0);
  }
  return az_span_slice(topic, 0, pos);
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

AZ_INLINE az_result
_az_mqtt5_rpc_verify_match_and_copy(az_span value, az_span extracted_token, az_span* out_token)
{
  if (az_span_is_content_equal(value, AZ_SPAN_EMPTY)
      || az_span_is_content_equal(value, extracted_token))
  {
    if (out_token)
    {
      *out_token = extracted_token;
    }
    return AZ_OK;
  }
  return AZ_ERROR_IOT_TOPIC_NO_MATCH;
}

AZ_NODISCARD az_result _az_mqtt5_topic_parser_replace_tokens_in_format(
    az_span mqtt_topic_span,
    az_span topic_format,
    az_span service_group_id,
    az_span client_id,
    az_span model_id,
    az_span executor_id,
    az_span sender_id,
    az_span command_name,
    az_span telemetry_name,
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
    if (*required_length > (uint32_t)az_span_size(mqtt_topic_span))
    {
      ret = AZ_ERROR_NOT_ENOUGH_SPACE;
    }
    else
    {
      remainder = az_span_copy(remainder, service_group_id_key);
      remainder = az_span_copy(remainder, service_group_id);
      remainder = az_span_copy_u8(remainder, '/');
    }
  }

  for (int32_t i = 0; i < az_span_size(topic_format); i++)
  {
    uint8_t c = az_span_ptr(topic_format)[i];
    if (++*required_length > (uint32_t)az_span_size(mqtt_topic_span))
    {
      ret = AZ_ERROR_NOT_ENOUGH_SPACE;
    }

    if (c == '{')
    {
      if (i + 1 >= az_span_size(topic_format))
      {
        return AZ_ERROR_ARG;
      }
      az_span token
          = _az_mqtt5_rpc_extract_next_token_from_format(az_span_slice_to_end(topic_format, i + 1));
      if (az_span_is_content_equal(token, AZ_SPAN_EMPTY))
      {
        return AZ_ERROR_ARG;
      }
      uint32_t curr_hash = _az_mqtt5_topic_parser_calculate_hash(token);
      if (curr_hash == _az_MQTT5_TOPIC_PARSER_CLIENT_ID_HASH)
      {
        ret = _az_mqtt5_rpc_verify_buffer_length_and_copy(
            client_id, &remainder, required_length, buffer_length);
        i += az_span_size(invoker_client_id_key) - 1;
      }
      else if (curr_hash == _az_MQTT5_TOPIC_PARSER_MODEL_ID_HASH)
      {
        ret = _az_mqtt5_rpc_verify_buffer_length_and_copy(
            model_id, &remainder, required_length, buffer_length);
        i += az_span_size(model_id_key) - 1;
      }
      else if (curr_hash == _az_MQTT5_TOPIC_PARSER_EXECUTOR_ID_HASH)
      {
        ret = _az_mqtt5_rpc_verify_buffer_length_and_copy(
            executor_id, &remainder, required_length, buffer_length);
        i += az_span_size(executor_client_id_key) - 1;
      }
      else if (curr_hash == _az_MQTT5_TOPIC_PARSER_SENDER_ID_HASH)
      {
        ret = _az_mqtt5_rpc_verify_buffer_length_and_copy(
            sender_id, &remainder, required_length, buffer_length);
        i += az_span_size(sender_id_key) - 1;
      }
      else if (curr_hash == _az_MQTT5_TOPIC_PARSER_COMMAND_NAME_HASH)
      {
        ret = _az_mqtt5_rpc_verify_buffer_length_and_copy(
            command_name, &remainder, required_length, buffer_length);
        i += az_span_size(command_name_key) - 1;
      }
      else if (curr_hash == _az_MQTT5_TOPIC_PARSER_TELEMETRY_NAME_HASH)
      {
        ret = _az_mqtt5_rpc_verify_buffer_length_and_copy(
            telemetry_name, &remainder, required_length, buffer_length);
        i += az_span_size(telemetry_name_key) - 1;
      }
      else
      {
        return AZ_ERROR_ARG;
      }

      if (ret == AZ_ERROR_ARG)
      {
        // If the topic format contains a topic token which the user failed to provide, we return an
        // error.
        return ret;
      }
    }
    else if (az_result_succeeded(ret))
    {
      remainder = az_span_copy_u8(remainder, c);
    }
  }

  if (az_result_succeeded(ret))
  {
    az_span_copy_u8(remainder, '\0');
  }

  return ret;
}

AZ_NODISCARD az_result _az_mqtt5_topic_parser_extract_tokens_from_topic(
    az_span topic_format,
    az_span received_topic,
    az_span client_id,
    az_span model_id,
    az_span executor_id,
    az_span sender_id,
    az_span* extracted_client_id,
    az_span* extracted_model_id,
    az_span* extracted_executor_id,
    az_span* extracted_sender_id,
    az_span* extracted_command_name,
    az_span* extracted_telemetry_name)
{
  if (extracted_client_id)
  {
    *extracted_client_id = AZ_SPAN_EMPTY;
  }
  if (extracted_model_id)
  {
    *extracted_model_id = AZ_SPAN_EMPTY;
  }
  if (extracted_executor_id)
  {
    *extracted_executor_id = AZ_SPAN_EMPTY;
  }
  if (extracted_sender_id)
  {
    *extracted_sender_id = AZ_SPAN_EMPTY;
  }
  if (extracted_command_name)
  {
    *extracted_command_name = AZ_SPAN_EMPTY;
  }
  if (extracted_telemetry_name)
  {
    *extracted_telemetry_name = AZ_SPAN_EMPTY;
  }

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
        if (format_idx + 1 >= az_span_size(topic_format) || i + 1 >= az_span_size(received_topic))
        {
          return AZ_ERROR_ARG;
        }
        az_span format_token = _az_mqtt5_rpc_extract_next_token_from_format(
            az_span_slice_to_end(topic_format, format_idx + 1));
        az_span extracted_token
            = _az_mqtt5_rpc_extract_next_token_from_topic(az_span_slice_to_end(received_topic, i));
        if (az_span_is_content_equal(format_token, AZ_SPAN_EMPTY))
        {
          return AZ_ERROR_ARG;
        }
        if (az_span_is_content_equal(extracted_token, AZ_SPAN_EMPTY))
        {
          return AZ_ERROR_IOT_TOPIC_NO_MATCH;
        }
        uint32_t curr_hash = _az_mqtt5_topic_parser_calculate_hash(format_token);

        if (curr_hash == _az_MQTT5_TOPIC_PARSER_CLIENT_ID_HASH)
        {
          _az_RETURN_IF_FAILED(
              _az_mqtt5_rpc_verify_match_and_copy(client_id, extracted_token, extracted_client_id));
          i += az_span_size(extracted_token) - 1;
          format_idx += az_span_size(invoker_client_id_key) - 1;
        }
        else if (curr_hash == _az_MQTT5_TOPIC_PARSER_MODEL_ID_HASH)
        {
          _az_RETURN_IF_FAILED(
              _az_mqtt5_rpc_verify_match_and_copy(model_id, extracted_token, extracted_model_id));
          i += az_span_size(extracted_token) - 1;
          format_idx += az_span_size(model_id_key) - 1;
        }
        else if (curr_hash == _az_MQTT5_TOPIC_PARSER_EXECUTOR_ID_HASH)
        {
          _az_RETURN_IF_FAILED(_az_mqtt5_rpc_verify_match_and_copy(
              executor_id, extracted_token, extracted_executor_id));
          i += az_span_size(extracted_token) - 1;
          format_idx += az_span_size(executor_client_id_key) - 1;
        }
        else if (curr_hash == _az_MQTT5_TOPIC_PARSER_SENDER_ID_HASH)
        {
          _az_RETURN_IF_FAILED(
              _az_mqtt5_rpc_verify_match_and_copy(sender_id, extracted_token, extracted_sender_id));
          i += az_span_size(extracted_token) - 1;
          format_idx += az_span_size(sender_id_key) - 1;
        }
        else if (curr_hash == _az_MQTT5_TOPIC_PARSER_COMMAND_NAME_HASH)
        {
          _az_RETURN_IF_FAILED(_az_mqtt5_rpc_verify_match_and_copy(
              AZ_SPAN_EMPTY, extracted_token, extracted_command_name));
          i += az_span_size(extracted_token) - 1;
          format_idx += az_span_size(command_name_key) - 1;
        }
        else if (curr_hash == _az_MQTT5_TOPIC_PARSER_TELEMETRY_NAME_HASH)
        {
          _az_RETURN_IF_FAILED(_az_mqtt5_rpc_verify_match_and_copy(
              AZ_SPAN_EMPTY, extracted_token, extracted_telemetry_name));
          i += az_span_size(extracted_token) - 1;
          format_idx += az_span_size(telemetry_name_key) - 1;
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

AZ_NODISCARD bool _az_mqtt5_topic_parser_valid_topic_format(az_span topic_format)
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

  int32_t telemetry_name_index = az_span_find(topic_format, telemetry_name_key);
  if (telemetry_name_index != -1)
  {
    if (!_az_mqtt5_is_key_surrounded_by_slash(
            telemetry_name_index, az_span_size(telemetry_name_key), topic_format))
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
