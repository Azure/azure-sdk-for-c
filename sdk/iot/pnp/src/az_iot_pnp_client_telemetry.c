// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_pnp_client.h"
#include <az_precondition.h>
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg.h>

static const uint8_t null_terminator = '\0';
static const uint8_t pnp_telemetry_param_separator = '&';
static const uint8_t pnp_telemetry_param_equals = '=';

static const az_span pnp_telemetry_component_name_param = AZ_SPAN_LITERAL_FROM_STR("%24.ifname");
static const az_span pnp_telemetry_content_type_param = AZ_SPAN_LITERAL_FROM_STR("%24.ct");
static const az_span pnp_telemetry_content_encoding_param = AZ_SPAN_LITERAL_FROM_STR("%24.ce");

static az_span _az_add_telemetry_property(
    az_span mqtt_topic,
    az_span property_name,
    az_span property_value,
    bool add_separator)
{
  int32_t required_length = az_span_size(property_name) + az_span_size(property_value) + 1;
  if (add_separator)
  {
    required_length++;
  }

  if (required_length > az_span_size(mqtt_topic))
  {
    return AZ_SPAN_NULL;
  }

  az_span remainder = mqtt_topic;
  if (add_separator)
  {
    remainder = az_span_copy_u8(remainder, pnp_telemetry_param_separator);
  }
  remainder = az_span_copy(remainder, property_name);
  remainder = az_span_copy_u8(remainder, pnp_telemetry_param_equals);
  az_span_copy(remainder, property_value);

  return az_span_slice_to_end(mqtt_topic, required_length);
}

AZ_NODISCARD az_result az_iot_pnp_client_telemetry_get_publish_topic(
    az_iot_pnp_client const* client,
    az_span component_name,
    void* reserved,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(component_name, 1, false);
  AZ_PRECONDITION_NOT_NULL(mqtt_topic);
  AZ_PRECONDITION(mqtt_topic_size > 0);
  AZ_PRECONDITION_IS_NULL(reserved);
  (void)reserved;

  size_t hub_topic_length;

  // First get hub topic
  AZ_RETURN_IF_FAILED(az_iot_hub_client_telemetry_get_publish_topic(
      &client->_internal.iot_hub_client,
      NULL,
      mqtt_topic,
      mqtt_topic_size,
      (size_t*)&hub_topic_length));

  az_span mqtt_topic_span = az_span_init((uint8_t*)mqtt_topic, (int32_t)mqtt_topic_size);

  // Hub topic plus pnp values
  int32_t required_length = (int32_t)hub_topic_length
      + az_span_size(pnp_telemetry_component_name_param)
      + (int32_t)sizeof(pnp_telemetry_param_equals) + az_span_size(component_name);

  // Content type size if applicable
  if (az_span_ptr(client->_internal.options.content_type) != NULL)
  {
    required_length += (int32_t)sizeof(pnp_telemetry_param_separator)
        + az_span_size(pnp_telemetry_content_type_param)
        + (int32_t)sizeof(pnp_telemetry_param_equals)
        + az_span_size(client->_internal.options.content_type);
  }

  // Content encoding size if applicable
  if (az_span_ptr(client->_internal.options.content_encoding) != NULL)
  {
    required_length += (int32_t)sizeof(pnp_telemetry_param_separator)
        + az_span_size(pnp_telemetry_content_encoding_param)
        + (int32_t)sizeof(pnp_telemetry_param_equals)
        + az_span_size(client->_internal.options.content_encoding);
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_topic_span, required_length + (int32_t)sizeof(null_terminator));

  // Proceed with appending since there is enough size
  az_span remaining = az_span_init(
      (uint8_t*)(mqtt_topic + hub_topic_length),
      (int32_t)(mqtt_topic_size - hub_topic_length));

  remaining = _az_add_telemetry_property(
      remaining, pnp_telemetry_component_name_param, component_name, false);

  if (az_span_ptr(client->_internal.options.content_type) != NULL)
  {
    remaining = _az_add_telemetry_property(
        remaining, pnp_telemetry_content_type_param, client->_internal.options.content_type, true);
  }

  if (az_span_ptr(client->_internal.options.content_encoding) != NULL)
  {
    remaining = _az_add_telemetry_property(
        remaining,
        pnp_telemetry_content_encoding_param,
        client->_internal.options.content_encoding,
        true);
  }

  az_span_copy_u8(remaining, null_terminator);

  if (out_mqtt_topic_length)
  {
    *out_mqtt_topic_length = (size_t)required_length;
  }

  return AZ_OK;
}
