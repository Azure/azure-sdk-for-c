// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_hub_client.h"
#include <az_contract_internal.h>
#include <az_result.h>
#include <az_span.h>

static const az_span telemetry_prop_delim = AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER((uint8_t[]){ '?' });
static const az_span telemetry_prop_separator
    = AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER((uint8_t[]){ '&' });
static const az_span telemetry_null_terminator
    = AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER((uint8_t[]){ '\0' });
static const az_span telemetry_topic_prefix = AZ_SPAN_LITERAL_FROM_STR("devices/");
static const az_span telemetry_topic_modules_mid = AZ_SPAN_LITERAL_FROM_STR("/modules/");
static const az_span telemetry_topic_suffix = AZ_SPAN_LITERAL_FROM_STR("/messages/events/");

az_result az_iot_hub_client_telemetry_publish_topic_get(
    az_iot_hub_client const* client,
    az_iot_hub_client_properties const* properties,
    az_span mqtt_topic,
    az_span* out_mqtt_topic)
{
  AZ_CONTRACT_ARG_NOT_NULL(client);
  AZ_CONTRACT_ARG_VALID_SPAN(mqtt_topic);
  AZ_CONTRACT_ARG_NOT_NULL(out_mqtt_topic);

  az_result result;

  int32_t required_size = 0;

  // Required topic parts
  required_size = az_span_length(telemetry_topic_prefix)
      + az_span_length(client->_internal.device_id) + az_span_length(telemetry_topic_suffix)
      + az_span_length(telemetry_null_terminator);

  // Optional parts
  if (!az_span_is_empty(client->_internal.options.module_id))
  {
    required_size += az_span_length(telemetry_topic_modules_mid);
    required_size += az_span_length(client->_internal.options.module_id);
  }
  if (properties != NULL)
  {
    required_size
        += az_span_length(properties->_internal.properties) + az_span_length(telemetry_prop_delim);
  }
  if (!az_span_is_empty(client->_internal.options.user_agent))
  {
    required_size += az_span_length(client->_internal.options.user_agent)
        + az_span_length(telemetry_prop_delim);
  }

  // Only build topic if the span has the capacity
  if (az_span_capacity(mqtt_topic) < required_size)
  {
    result = AZ_ERROR_OUT_OF_MEMORY;
  }
  else
  {
    AZ_RETURN_IF_FAILED(az_span_append(mqtt_topic, telemetry_topic_prefix, out_mqtt_topic));
    AZ_RETURN_IF_FAILED(
        az_span_append(*out_mqtt_topic, client->_internal.device_id, out_mqtt_topic));

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
      AZ_RETURN_IF_FAILED(az_span_append(*out_mqtt_topic, telemetry_prop_delim, out_mqtt_topic));
      AZ_RETURN_IF_FAILED(
          az_span_append(*out_mqtt_topic, properties->_internal.properties, out_mqtt_topic));
    }
    if (az_span_length(client->_internal.options.user_agent) != 0)
    {
      AZ_RETURN_IF_FAILED(
          properties == NULL
              ? az_span_append(*out_mqtt_topic, telemetry_prop_delim, out_mqtt_topic)
              : az_span_append(*out_mqtt_topic, telemetry_prop_separator, out_mqtt_topic));
      AZ_RETURN_IF_FAILED(
          az_span_append(*out_mqtt_topic, client->_internal.options.user_agent, out_mqtt_topic));
    }
    AZ_RETURN_IF_FAILED(az_span_append(*out_mqtt_topic, telemetry_null_terminator, out_mqtt_topic));
    result = AZ_OK;
  }

  return result;
}
