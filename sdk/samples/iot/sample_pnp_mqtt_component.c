// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "sample_pnp_mqtt_component.h"

#include <stddef.h>
#include <stdint.h>

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

static uint32_t request_id_int;
static char request_id_buf[10];

static char publish_topic[128];
static char publish_payload[512];

// Create request id span which increments request id integer each call. Capable of holding 8 digit
// number.
az_span get_request_id(void)
{
  az_span remainder;
  az_span out_span = az_span_create((uint8_t*)request_id_buf, sizeof(request_id_buf));

  // Note that if left to run for a long time, this will overflow and reset back to 0.
  az_result result = az_span_u32toa(out_span, request_id_int++, &remainder);
  (void)result;

  return az_span_slice(out_span, 0, az_span_size(out_span) - az_span_size(remainder));
}

az_result pnp_mqtt_message_init(pnp_mqtt_message* mqtt_message)
{
  if (mqtt_message == NULL)
  {
    return AZ_ERROR_ARG;
  }

  mqtt_message->topic = publish_topic;
  mqtt_message->topic_length = sizeof(publish_topic);
  mqtt_message->out_topic_length = 0;
  mqtt_message->payload_span = AZ_SPAN_FROM_BUFFER(publish_payload);
  mqtt_message->out_payload_span = mqtt_message->payload_span;

  return AZ_OK;
}
