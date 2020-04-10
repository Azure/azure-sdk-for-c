// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_hub_client.h"
#include <az_precondition.h>
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg.h>

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
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic);

  const az_span* const module_id = &(client->_internal.options.module_id);

  int32_t required_length = az_span_length(telemetry_topic_prefix)
      + az_span_length(client->_internal.device_id) + az_span_length(telemetry_topic_suffix);
  int32_t module_id_length = az_span_length(*module_id);
  if (module_id_length > 0)
  {
    required_length += az_span_length(telemetry_topic_modules_mid) + az_span_length(*module_id);
  }
  if (properties != NULL)
  {
    required_length += az_span_length(properties->_internal.properties);
  }

  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(mqtt_topic, required_length);

  mqtt_topic = az_span_copy(mqtt_topic, telemetry_topic_prefix);
  mqtt_topic = az_span_append(mqtt_topic, client->_internal.device_id);

  if (module_id_length > 0)
  {
    mqtt_topic = az_span_append(mqtt_topic, telemetry_topic_modules_mid);
    mqtt_topic = az_span_append(mqtt_topic, *module_id);
  }

  mqtt_topic = az_span_append(mqtt_topic, telemetry_topic_suffix);

  if (properties != NULL)
  {
    mqtt_topic = az_span_append(mqtt_topic, properties->_internal.properties);
  }

  *out_mqtt_topic = mqtt_topic;

  return AZ_OK;
}
