// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef SAMPLE_PNP_MQTT_COMPONENT_H
#define SAMPLE_PNP_MQTT_COMPONENT_H

#include <stddef.h>

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

typedef struct
{
  char* topic;
  size_t topic_length;
  size_t* out_topic_length;
  az_span payload_span;
  az_span out_payload_span;
} pnp_mqtt_message;

void pnp_mqtt_message_init(pnp_mqtt_message* mqtt_message);

// Create request id span which increments request id integer each call. Capable of holding 8 digit
// number.
az_span get_request_id(void);

#endif // SAMPLE_PNP_MQTT_COMPONENT_H
