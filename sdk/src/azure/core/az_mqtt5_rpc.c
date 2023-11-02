// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5_rpc.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
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
