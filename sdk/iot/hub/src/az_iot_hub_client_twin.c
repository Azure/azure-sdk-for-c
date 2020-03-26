// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_hub_client.h"
#include <az_precondition.h>
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg.h>

static const uint8_t az_iot_hub_client_twin_hashtag = '#';
static const az_span az_iot_hub_client_twin_request_id_suffix = AZ_SPAN_LITERAL_FROM_STR("?$rid=");

static const az_span az_iot_hub_twin_response_sub_topic
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/twin/res/");
static const az_span az_iot_hub_twin_get_pub_topic = AZ_SPAN_LITERAL_FROM_STR("$iothub/twin/GET/");
static const az_span az_iot_hub_twin_patch_pub_topic
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/twin/PATCH/properties/reported/");
static const az_span az_iot_hub_twin_patch_sub_topic
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/twin/PATCH/properties/desired/");

AZ_NODISCARD az_result az_iot_hub_client_twin_response_subscribe_topic_filter_get(
    az_iot_hub_client const* client,
    az_span mqtt_topic_filter,
    az_span* out_mqtt_topic_filter)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic_filter, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic_filter);

  AZ_RETURN_IF_FAILED(
      az_span_append(mqtt_topic_filter, az_iot_hub_twin_response_sub_topic, &mqtt_topic_filter));
  AZ_RETURN_IF_FAILED(
      az_span_append_uint8(mqtt_topic_filter, az_iot_hub_client_twin_hashtag, &mqtt_topic_filter));

  *out_mqtt_topic_filter = mqtt_topic_filter;

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_twin_patch_subscribe_topic_filter_get(
    az_iot_hub_client const* client,
    az_span mqtt_topic_filter,
    az_span* out_mqtt_topic_filter)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic_filter, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic_filter);

  AZ_RETURN_IF_FAILED(
      az_span_append(mqtt_topic_filter, az_iot_hub_twin_patch_sub_topic, &mqtt_topic_filter));

  *out_mqtt_topic_filter = mqtt_topic_filter;

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_twin_get_publish_topic_get(
    az_iot_hub_client const* client,
    az_span request_id,
    az_span mqtt_topic,
    az_span* out_mqtt_topic)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(request_id, 1, false);
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic);

  AZ_RETURN_IF_FAILED(az_span_append(mqtt_topic, az_iot_hub_twin_get_pub_topic, &mqtt_topic));
  AZ_RETURN_IF_FAILED(az_span_append(mqtt_topic, az_iot_hub_client_twin_request_id_suffix, &mqtt_topic));
  AZ_RETURN_IF_FAILED(az_span_append(mqtt_topic, request_id, &mqtt_topic));

  *out_mqtt_topic = mqtt_topic;

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_twin_patch_publish_topic_get(
    az_iot_hub_client const* client,
    az_span request_id,
    az_span mqtt_topic,
    az_span* out_mqtt_topic)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(request_id, 1, false);
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic);

  AZ_RETURN_IF_FAILED(az_span_append(mqtt_topic, az_iot_hub_twin_patch_pub_topic, &mqtt_topic));
  AZ_RETURN_IF_FAILED(az_span_append(mqtt_topic, az_iot_hub_client_twin_request_id_suffix, &mqtt_topic));
  AZ_RETURN_IF_FAILED(az_span_append(mqtt_topic, request_id, &mqtt_topic));

  *out_mqtt_topic = mqtt_topic;

  return AZ_OK;
}
