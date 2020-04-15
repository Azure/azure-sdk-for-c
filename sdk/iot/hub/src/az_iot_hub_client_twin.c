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

  int32_t required_length = az_span_size(az_iot_hub_twin_response_sub_topic) + 1;

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_topic_filter, required_length);

  az_span remainder = az_span_copy(mqtt_topic_filter, az_iot_hub_twin_response_sub_topic);
  az_span_copy_uint8(remainder, az_iot_hub_client_twin_hashtag);

  *out_mqtt_topic_filter = az_span_slice(mqtt_topic_filter, 0, required_length);

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

  int32_t required_length = az_span_size(az_iot_hub_twin_patch_sub_topic);

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_topic_filter, required_length);

  az_span_copy(mqtt_topic_filter, az_iot_hub_twin_patch_sub_topic);

  *out_mqtt_topic_filter = az_span_slice(mqtt_topic_filter, 0, required_length);

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

  int32_t required_length = az_span_size(az_iot_hub_twin_get_pub_topic)
      + az_span_size(az_iot_hub_client_twin_request_id_suffix) + az_span_size(request_id);

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_topic, required_length);

  az_span remainder = az_span_copy(mqtt_topic, az_iot_hub_twin_get_pub_topic);
  remainder = az_span_copy(remainder, az_iot_hub_client_twin_request_id_suffix);
  az_span_copy(remainder, request_id);

  *out_mqtt_topic = az_span_slice(mqtt_topic, 0, required_length);

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

  int32_t required_length = az_span_size(az_iot_hub_twin_patch_pub_topic)
      + az_span_size(az_iot_hub_client_twin_request_id_suffix) + az_span_size(request_id);

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_topic, required_length);

  az_span remainder = az_span_copy(mqtt_topic, az_iot_hub_twin_patch_pub_topic);
  remainder = az_span_copy(remainder, az_iot_hub_client_twin_request_id_suffix);
  az_span_copy(remainder, request_id);

  *out_mqtt_topic = az_span_slice(mqtt_topic, 0, required_length);

  return AZ_OK;
}
