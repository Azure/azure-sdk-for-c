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

/**
 * @brief helper function to check if an #az_mqtt5_rpc_status indicates failure
 *
 * @param[in] status the status to check
 *
 * @return true if the status indicates failure, false otherwise
 */
AZ_NODISCARD bool az_mqtt5_rpc_status_failed(az_mqtt5_rpc_status status)
{
  return (status < 200 || status >= 300);
}

AZ_NODISCARD az_result az_rpc_get_topic_from_format(az_span model_id, az_span executor_client_id, az_span invoker_client_id, az_span command_name, az_span format, az_span out_topic,
    int32_t* out_topic_length)
{
  const az_span service_id_key = AZ_SPAN_FROM_STR("{serviceId}");
  const az_span command_name_key = AZ_SPAN_FROM_STR("{name}");
  const az_span executor_id_key = AZ_SPAN_FROM_STR("{executorId}");
  const az_span invoker_id_key = AZ_SPAN_FROM_STR("{invokerId}");

  int32_t format_size = az_span_size(format);

  // Determine the length of the final topic
  int32_t topic_length = format_size;
  if(az_span_find(format, service_id_key) >= 0)
  {
    _az_PRECONDITION_VALID_SPAN(model_id, 1, false);
    topic_length += az_span_size(model_id);
    topic_length -= az_span_size(service_id_key);
  }
  if(az_span_find(format, command_name_key) >= 0)
  {
    _az_PRECONDITION_VALID_SPAN(command_name, 1, false);
    topic_length += az_span_size(command_name);
    topic_length -= az_span_size(command_name_key);
  }
  if(az_span_find(format, executor_id_key) >= 0)
  {
    _az_PRECONDITION_VALID_SPAN(executor_client_id, 1, false);
    topic_length += az_span_size(executor_client_id);
    topic_length -= az_span_size(executor_id_key);
  }
  if(az_span_find(format, invoker_id_key) >= 0)
  {
    _az_PRECONDITION_VALID_SPAN(invoker_client_id, 1, false);
    topic_length += az_span_size(invoker_client_id);
    topic_length -= az_span_size(invoker_id_key);
  }

  _az_PRECONDITION_VALID_SPAN(out_topic, topic_length, true);

  char format_buf[az_span_size(out_topic)];
  az_span temp_format_buf = AZ_SPAN_FROM_BUFFER(format_buf);
  az_span_copy(temp_format_buf, format);
  temp_format_buf = az_span_slice(temp_format_buf, 0, format_size);

  az_span temp_span = out_topic;
  
  int32_t index = az_span_find(temp_format_buf, service_id_key);
  if (index >= 0)
  {
    format_size += az_span_size(model_id);
    format_size -= az_span_size(service_id_key);

    temp_span = az_span_copy(
        temp_span, az_span_slice(temp_format_buf, 0, index));
    temp_span = az_span_copy(temp_span, model_id);
    temp_span = az_span_copy(
        temp_span, az_span_slice_to_end(temp_format_buf, index + az_span_size(service_id_key)));

    temp_format_buf = AZ_SPAN_FROM_BUFFER(format_buf);
    az_span_copy(temp_format_buf, out_topic);
    temp_format_buf = az_span_slice(temp_format_buf, 0, format_size);
    temp_span = out_topic;
  }

  index = az_span_find(temp_format_buf, command_name_key);
  if (index >= 0)
  {
    format_size += az_span_size(command_name);
    format_size -= az_span_size(command_name_key);

    temp_span = az_span_copy(
        temp_span, az_span_slice(temp_format_buf, 0, index));
    temp_span = az_span_copy(temp_span, command_name);
    temp_span = az_span_copy(
        temp_span, az_span_slice_to_end(temp_format_buf, index + az_span_size(command_name_key)));

    temp_format_buf = AZ_SPAN_FROM_BUFFER(format_buf);
    az_span_copy(temp_format_buf, out_topic);
    temp_format_buf = az_span_slice(temp_format_buf, 0, format_size);
    temp_span = out_topic;
  }

  index = az_span_find(temp_format_buf, executor_id_key);
  if (index >= 0)
  {
    format_size += az_span_size(executor_client_id);
    format_size -= az_span_size(executor_id_key);

    temp_span = az_span_copy(
        temp_span, az_span_slice(temp_format_buf, 0, index));
    temp_span = az_span_copy(temp_span, executor_client_id);
    temp_span = az_span_copy(
        temp_span, az_span_slice_to_end(temp_format_buf, index + az_span_size(executor_id_key)));

    temp_format_buf = AZ_SPAN_FROM_BUFFER(format_buf);
    az_span_copy(temp_format_buf, out_topic);
    temp_format_buf = az_span_slice(temp_format_buf, 0, format_size);
    temp_span = out_topic;
  }

  index = az_span_find(temp_format_buf, invoker_id_key);
  if (index >= 0)
  {
    format_size += az_span_size(invoker_client_id);
    format_size -= az_span_size(invoker_id_key);

    temp_span = az_span_copy(
        temp_span, az_span_slice(temp_format_buf, 0, index));
    temp_span = az_span_copy(temp_span, invoker_client_id);
    temp_span = az_span_copy(
        temp_span, az_span_slice_to_end(temp_format_buf, index + az_span_size(invoker_id_key)));

    temp_format_buf = AZ_SPAN_FROM_BUFFER(format_buf);
    az_span_copy(temp_format_buf, out_topic);
    temp_format_buf = az_span_slice(temp_format_buf, 0, format_size);
    temp_span = out_topic;
  }

  if (out_topic_length != NULL)
  {
    *out_topic_length = topic_length;
  }

  return AZ_OK;
}