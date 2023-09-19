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

static const az_span model_id_key = AZ_SPAN_LITERAL_FROM_STR("{serviceId}");
static const az_span command_name_key = AZ_SPAN_LITERAL_FROM_STR("{name}");
static const az_span executor_client_id_key = AZ_SPAN_LITERAL_FROM_STR("{executorId}");
static const az_span invoker_client_id_key = AZ_SPAN_LITERAL_FROM_STR("{invokerId}");

AZ_NODISCARD int32_t
_az_rpc_calculate_substitution_length(az_span format, az_span key, az_span value);
AZ_NODISCARD az_result _az_rpc_substitute_key_for_value(
    az_span format,
    uint8_t* format_buf,
    az_span key,
    az_span value,
    az_span out_topic);

AZ_NODISCARD bool az_span_topic_matches_sub(az_span sub, az_span topic)
{
  bool ret;
#if defined(TRANSPORT_MOSQUITTO)
  if (MOSQ_ERR_SUCCESS
      != mosquitto_topic_matches_sub((char*)az_span_ptr(sub), (char*)az_span_ptr(topic), &ret))
  {
    ret = false;
  }
#else // TRANSPORT_MOSQUITTO
  (void)sub;
  (void)topic;
  ret = false;
#endif
  return ret;
}

AZ_NODISCARD bool az_mqtt5_rpc_status_failed(az_mqtt5_rpc_status status)
{
  return (status < 200 || status >= 300);
}

AZ_NODISCARD int32_t
_az_rpc_calculate_substitution_length(az_span format, az_span key, az_span value)
{
  if (az_span_find(format, key) >= 0)
  {
    _az_PRECONDITION_VALID_SPAN(value, 1, false);
    return az_span_size(value) - az_span_size(key);
  }
  return 0;
}

AZ_NODISCARD az_result _az_rpc_substitute_key_for_value(
    az_span format,
    uint8_t* format_buf,
    az_span key,
    az_span value,
    az_span out_topic)
{
  az_span temp_span = out_topic;

  int32_t index = az_span_find(format, key);
  if (index >= 0)
  {
    // make a copy of the format to copy from
    az_span temp_format_buf = az_span_create(format_buf, az_span_size(format));
    az_span_copy(temp_format_buf, format);

    temp_span = az_span_copy(temp_span, az_span_slice(temp_format_buf, 0, index));
    temp_span = az_span_copy(temp_span, value);
    temp_span
        = az_span_copy(temp_span, az_span_slice_to_end(temp_format_buf, index + az_span_size(key)));
    return AZ_OK;
  }
  return AZ_ERROR_ITEM_NOT_FOUND;
}

AZ_NODISCARD az_result az_rpc_get_topic_from_format(
    az_span format,
    az_span model_id,
    az_span executor_client_id,
    az_span invoker_client_id,
    az_span command_name,
    az_span out_topic,
    int32_t* out_topic_length)
{
  int32_t format_size = az_span_size(format);

  // Determine the length of the final topic and the max length of the topic while performing
  // substitutions
  int32_t topic_length = format_size + 1; // + 1 for the null terminator
  int32_t max_temp_length = 0;
  if ((topic_length += _az_rpc_calculate_substitution_length(format, model_id_key, model_id))
      > max_temp_length)
  {
    max_temp_length = topic_length;
  }
  if ((topic_length
       += _az_rpc_calculate_substitution_length(format, command_name_key, command_name))
      > max_temp_length)
  {
    max_temp_length = topic_length;
  }
  if ((topic_length
       += _az_rpc_calculate_substitution_length(format, executor_client_id_key, executor_client_id))
      > max_temp_length)
  {
    max_temp_length = topic_length;
  }
  if ((topic_length
       += _az_rpc_calculate_substitution_length(format, invoker_client_id_key, invoker_client_id))
      > max_temp_length)
  {
    max_temp_length = topic_length;
  }

  // Must be large enough to fit the entire format as items are substituted throughout the function
  // even if that's larger than the output topic
  _az_PRECONDITION_VALID_SPAN(out_topic, max_temp_length, true);

  // Must be large enough to fit the entire format even if that's larger than the output topic
  int32_t format_buf_size
      = format_size > az_span_size(out_topic) ? format_size : az_span_size(out_topic);
  uint8_t format_buf[format_buf_size];

  if (az_result_succeeded(
          _az_rpc_substitute_key_for_value(format, format_buf, model_id_key, model_id, out_topic)))
  {
    format_size += az_span_size(model_id);
    format_size -= az_span_size(model_id_key);
    format = az_span_slice(out_topic, 0, format_size);
  }

  if (az_result_succeeded(_az_rpc_substitute_key_for_value(
          format, format_buf, command_name_key, command_name, out_topic)))
  {
    format_size += az_span_size(command_name);
    format_size -= az_span_size(command_name_key);
    format = az_span_slice(out_topic, 0, format_size);
  }

  if (az_result_succeeded(_az_rpc_substitute_key_for_value(
          format, format_buf, executor_client_id_key, executor_client_id, out_topic)))
  {
    format_size += az_span_size(executor_client_id);
    format_size -= az_span_size(executor_client_id_key);
    format = az_span_slice(out_topic, 0, format_size);
  }

  if (az_result_succeeded(_az_rpc_substitute_key_for_value(
          format, format_buf, invoker_client_id_key, invoker_client_id, out_topic)))
  {
    format_size += az_span_size(invoker_client_id);
    format_size -= az_span_size(invoker_client_id_key);
    format = az_span_slice(out_topic, 0, format_size);
  }

  // add null terminator to end of topic
  az_span temp_topic = out_topic;
  temp_topic = az_span_copy(temp_topic, format);
  az_span_copy_u8(temp_topic, '\0');

  if (out_topic_length != NULL)
  {
    *out_topic_length = topic_length;
  }

  return AZ_OK;
}
