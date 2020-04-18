// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_pnp_client.h"
#include <az_precondition.h>
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg.h>

static const uint8_t pnp_telemetry_param_separator = '&';
static const uint8_t pnp_telemetry_param_equals = '=';

static const az_span pnp_telemetry_component_name_param = AZ_SPAN_LITERAL_FROM_STR("%24.ifname");
static const az_span pnp_telemetry_content_type_param = AZ_SPAN_LITERAL_FROM_STR("%24.ct");
static const az_span pnp_telemetry_content_encoding_param = AZ_SPAN_LITERAL_FROM_STR("%24.ce");

static az_result _az_add_telemetry_property(
    az_span mqtt_topic,
    az_span property_name,
    az_span property_value,
    bool add_separator,
    az_span* out_mqtt_topic)
{
  int32_t required_length = az_span_size(property_name) + az_span_size(property_value) + 1;
  if (add_separator)
  {
    required_length++;
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_topic, required_length);

  az_span remainder = mqtt_topic;
  if (add_separator)
  {
    remainder = az_span_copy_u8(remainder, pnp_telemetry_param_separator);
  }
  remainder = az_span_copy(remainder, property_name);
  remainder = az_span_copy_u8(remainder, pnp_telemetry_param_equals);
  az_span_copy(remainder, property_value);

  *out_mqtt_topic = az_span_slice(mqtt_topic, 0, required_length);

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_pnp_client_telemetry_get_publish_topic(
    az_iot_pnp_client const* client,
    az_span component_name,
    az_span mqtt_topic,
    void* reserved,
    az_span* out_mqtt_topic)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(component_name, 1, false);
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic, 0, false);
  AZ_PRECONDITION_IS_NULL(reserved);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic);
  (void)reserved;

  az_span written_span;
  int32_t written = 0;

  AZ_RETURN_IF_FAILED(az_iot_hub_client_telemetry_publish_topic_get(
      &client->_internal.iot_hub_client,
      NULL,
      az_span_slice_to_end(mqtt_topic, written),
      &written_span));

  written += az_span_size(written_span);

  AZ_RETURN_IF_FAILED(_az_add_telemetry_property(
      az_span_slice_to_end(mqtt_topic, written),
      pnp_telemetry_component_name_param,
      component_name,
      false,
      &written_span));

  written += az_span_size(written_span);

  if (az_span_ptr(client->_internal.options.content_type) != NULL)
  {
    AZ_RETURN_IF_FAILED(_az_add_telemetry_property(
        az_span_slice_to_end(mqtt_topic, written),
        pnp_telemetry_content_type_param,
        client->_internal.options.content_type,
        true,
        &written_span));

    written += az_span_size(written_span);
  }

  if (az_span_ptr(client->_internal.options.content_encoding) != NULL)
  {
    AZ_RETURN_IF_FAILED(_az_add_telemetry_property(
        az_span_slice_to_end(mqtt_topic, written),
        pnp_telemetry_content_encoding_param,
        client->_internal.options.content_encoding,
        true,
        &written_span));

    written += az_span_size(written_span);
  }

  *out_mqtt_topic = az_span_slice(mqtt_topic, 0, written);

  return AZ_OK;
}
