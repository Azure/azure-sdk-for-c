// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_hub_client.h"
#include <az_precondition.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg.h>

static const uint8_t telemetry_prop_delim = '?';
static const uint8_t telemetry_prop_separator = '&';
static const uint8_t telemetry_null_terminator = '\0';
static const az_span telemetry_topic_prefix = AZ_SPAN_LITERAL_FROM_STR("devices/");
static const az_span telemetry_topic_modules_mid = AZ_SPAN_LITERAL_FROM_STR("/modules/");
static const az_span telemetry_topic_suffix = AZ_SPAN_LITERAL_FROM_STR("/messages/events/");

AZ_NODISCARD az_result az_iot_hub_client_telemetry_publish_topic_get(
    az_iot_hub_client const* client,
    az_iot_hub_client_properties const* properties,
    az_span mqtt_topic,
    az_span* out_mqtt_topic)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic, 0);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic);

  // Required topic parts
  int32_t required_size = az_span_length(telemetry_topic_prefix)
      + az_span_length(client->_internal.device_id) + az_span_length(telemetry_topic_suffix)
      + sizeof(telemetry_null_terminator);

  // Optional parts
  if (az_span_ptr(client->_internal.options.module_id) != NULL)
  {
    required_size += az_span_length(telemetry_topic_modules_mid);
    required_size += az_span_length(client->_internal.options.module_id);
  }

  if (properties != NULL)
  {
    required_size
        += az_span_length(properties->_internal.properties) + sizeof(telemetry_prop_delim);
  }

  if (az_span_ptr(client->_internal.options.user_agent) != NULL)
  {
    required_size
        += az_span_length(client->_internal.options.user_agent) + sizeof(telemetry_prop_delim);
  }

  // Only build topic if the span has the capacity
  if (az_span_capacity(mqtt_topic) < required_size)
  {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  // Build topic string
  AZ_RETURN_IF_FAILED(az_span_append(mqtt_topic, telemetry_topic_prefix, out_mqtt_topic));
  AZ_RETURN_IF_FAILED(az_span_append(*out_mqtt_topic, client->_internal.device_id, out_mqtt_topic));

  if (az_span_length(client->_internal.options.module_id) != 0)
  {
    AZ_RETURN_IF_FAILED(
        az_span_append(*out_mqtt_topic, telemetry_topic_modules_mid, out_mqtt_topic));
    AZ_RETURN_IF_FAILED(
        az_span_append(*out_mqtt_topic, client->_internal.options.module_id, out_mqtt_topic));
  }

  AZ_RETURN_IF_FAILED(az_span_append(*out_mqtt_topic, telemetry_topic_suffix, out_mqtt_topic));

  if (properties != NULL)
  {
    AZ_RETURN_IF_FAILED(
        az_span_append_uint8(*out_mqtt_topic, telemetry_prop_delim, out_mqtt_topic));
    AZ_RETURN_IF_FAILED(
        az_span_append(*out_mqtt_topic, properties->_internal.properties, out_mqtt_topic));
  }

  if (az_span_length(client->_internal.options.user_agent) != 0)
  {
    AZ_RETURN_IF_FAILED(
        properties == NULL
            ? az_span_append_uint8(*out_mqtt_topic, telemetry_prop_delim, out_mqtt_topic)
            : az_span_append_uint8(*out_mqtt_topic, telemetry_prop_separator, out_mqtt_topic));
    AZ_RETURN_IF_FAILED(
        az_span_append(*out_mqtt_topic, client->_internal.options.user_agent, out_mqtt_topic));
  }

  AZ_RETURN_IF_FAILED(
      az_span_append_uint8(*out_mqtt_topic, telemetry_null_terminator, out_mqtt_topic));

  return AZ_OK;
}
