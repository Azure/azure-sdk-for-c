// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5_telemetry.h>
#include <azure/core/az_mqtt5_telemetry_producer_codec.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_mqtt5_topic_parser_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_mqtt5_telemetry_producer_codec_options
az_mqtt5_telemetry_producer_codec_options_default()
{
  return (az_mqtt5_telemetry_producer_codec_options){
    .telemetry_topic_format = AZ_SPAN_LITERAL_FROM_STR(AZ_MQTT5_TELEMETRY_DEFAULT_TOPIC_FORMAT)
  };
}

AZ_NODISCARD az_result az_mqtt5_telemetry_producer_codec_get_publish_topic(
    az_mqtt5_telemetry_producer_codec* producer,
    az_span telemetry_name,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  _az_PRECONDITION_NOT_NULL(producer);
  _az_PRECONDITION_NOT_NULL(mqtt_topic);
  _az_PRECONDITION_RANGE(1, mqtt_topic_size, INT32_MAX);

  az_result ret = AZ_OK;

  az_span mqtt_topic_span = az_span_create((uint8_t*)mqtt_topic, (int32_t)mqtt_topic_size);
  uint32_t required_length = 0;

  ret = _az_mqtt5_topic_parser_replace_tokens_in_format(
      mqtt_topic_span,
      producer->_internal.options.telemetry_topic_format,
      AZ_SPAN_EMPTY,
      AZ_SPAN_EMPTY,
      producer->_internal.model_id,
      AZ_SPAN_EMPTY,
      producer->_internal.client_id,
      telemetry_name,
      AZ_SPAN_EMPTY,
      &required_length);

  if (out_mqtt_topic_length)
  {
    *out_mqtt_topic_length = (size_t)required_length;
  }

  return ret;
}

AZ_NODISCARD az_result az_mqtt5_telemetry_producer_codec_init(
    az_mqtt5_telemetry_producer_codec* producer,
    az_span model_id,
    az_span client_id,
    az_mqtt5_telemetry_producer_codec_options* options)
{
  _az_PRECONDITION_NOT_NULL(producer);

  if (options == NULL)
  {
    producer->_internal.options = az_mqtt5_telemetry_producer_codec_options_default();
  }
  else if (_az_mqtt5_topic_parser_valid_topic_format(options->telemetry_topic_format))
  {
    producer->_internal.options = *options;
  }
  else
  {
    return AZ_ERROR_ARG;
  }

  producer->_internal.model_id = model_id;
  producer->_internal.client_id = client_id;

  return AZ_OK;
}
