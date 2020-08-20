// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include <azure/core/az_precondition.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/az_iot_hub_client.h>

#include <azure/core/_az_cfg.h>

static const uint8_t null_terminator = '\0';
static const uint8_t telemetry_param_separator = '&';
static const az_span telemetry_topic_prefix = AZ_SPAN_LITERAL_FROM_STR("devices/");
static const az_span telemetry_topic_modules_mid = AZ_SPAN_LITERAL_FROM_STR("/modules/");
static const az_span telemetry_topic_suffix = AZ_SPAN_LITERAL_FROM_STR("/messages/events/");
static const az_span telemetry_content_type = AZ_SPAN_LITERAL_FROM_STR("ct=");
static const az_span telemetry_content_encoding = AZ_SPAN_LITERAL_FROM_STR("ce=");

AZ_NODISCARD az_result az_iot_hub_client_telemetry_get_publish_topic(
    az_iot_hub_client const* client,
    az_iot_hub_client_properties const* properties,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(mqtt_topic);
  _az_PRECONDITION(mqtt_topic_size > 0);

  const az_span* const module_id = &(client->_internal.options.module_id);

  az_span mqtt_topic_span = az_span_create((uint8_t*)mqtt_topic, (int32_t)mqtt_topic_size);
  int32_t required_length = az_span_size(telemetry_topic_prefix)
      + az_span_size(client->_internal.device_id) + az_span_size(telemetry_topic_suffix);
  int32_t module_id_length = az_span_size(*module_id);
  int32_t content_type_length = az_span_size(client->_internal.options.content_type);
  int32_t content_encoding_length = az_span_size(client->_internal.options.content_encoding);
  if (module_id_length > 0)
  {
    required_length += az_span_size(telemetry_topic_modules_mid) + module_id_length;
  }
  if (content_type_length > 0)
  {
    required_length += az_span_size(telemetry_content_type) + content_type_length;
  }
  if (content_encoding_length > 0)
  {
    required_length += az_span_size(telemetry_content_encoding) + content_encoding_length
        + (content_type_length > 0 ? (int32_t)sizeof(telemetry_param_separator) : 0);
  }
  if (properties != NULL)
  {
    required_length += properties->_internal.properties_written
        + ((content_type_length > 0 || content_encoding_length > 0)
               ? (int32_t)sizeof(telemetry_param_separator)
               : 0);
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_topic_span, required_length + (int32_t)sizeof(null_terminator));

  az_span remainder = az_span_copy(mqtt_topic_span, telemetry_topic_prefix);
  remainder = az_span_copy(remainder, client->_internal.device_id);

  if (module_id_length > 0)
  {
    remainder = az_span_copy(remainder, telemetry_topic_modules_mid);
    remainder = az_span_copy(remainder, *module_id);
  }

  remainder = az_span_copy(remainder, telemetry_topic_suffix);

  if (az_span_size(client->_internal.options.content_type) > 0)
  {
    remainder = az_span_copy(remainder, telemetry_content_type);
    remainder = az_span_copy(remainder, client->_internal.options.content_type);
    }

  if (content_encoding_length > 0)
  {
    if (content_type_length > 0)
    {
      remainder = az_span_copy_u8(remainder, telemetry_param_separator);
    }
    remainder = az_span_copy(remainder, telemetry_content_encoding);
    remainder = az_span_copy(remainder, client->_internal.options.content_encoding);
  }

  if (properties != NULL)
  {
    if (content_encoding_length > 0 || content_type_length > 0)
    {
      remainder = az_span_copy_u8(remainder, telemetry_param_separator);
    }
    remainder = az_span_copy(
        remainder,
        az_span_slice(
            properties->_internal.properties_buffer, 0, properties->_internal.properties_written));
  }

  az_span_copy_u8(remainder, null_terminator);

  if (out_mqtt_topic_length)
  {
    *out_mqtt_topic_length = (size_t)required_length;
  }

  return AZ_OK;
}
