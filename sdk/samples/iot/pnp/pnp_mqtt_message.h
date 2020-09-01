// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef PNP_MQTT_MESSAGE_H
#define PNP_MQTT_MESSAGE_H

#include <stddef.h>

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#define MQTT_TIMEOUT_RECEIVE_MAX_MESSAGE_COUNT 3
#define MQTT_TIMEOUT_RECEIVE_MS (8 * 1000)
#define MQTT_TIMEOUT_DISCONNECT_MS (10 * 1000)

typedef struct
{
  char* topic;
  size_t topic_length;
  size_t* out_topic_length;
  az_span payload_span;
  az_span out_payload_span;
} pnp_mqtt_message;

az_result pnp_mqtt_message_init(pnp_mqtt_message* mqtt_message);

static void pnp_mqtt_publish_message(const char* topic, az_span payload, int qos);
static void pnp_mqtt_receive_message(void);

// Create request id span which increments request id integer each call. Capable of holding 8 digit
// number.
az_span get_request_id(void);

#endif // PNP_MQTT_MESSAGE_H
