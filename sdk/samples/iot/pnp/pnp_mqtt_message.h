// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef PNP_MQTT_MESSAGE_H
#define PNP_MQTT_MESSAGE_H

#include <stddef.h>

#include <azure/az_core.h>

#define PNP_MQTT_TIMEOUT_RECEIVE_MAX_MESSAGE_COUNT 3
#define PNP_MQTT_TIMEOUT_RECEIVE_MS (8 * 1000)
#define PNP_MQTT_TIMEOUT_DISCONNECT_MS (10 * 1000)

typedef struct
{
  char* topic;
  size_t* out_topic_length;
  size_t topic_length;
  az_span out_payload;
  az_span payload;
} pnp_mqtt_message;

/**
 * @brief Initialize a #pnp_mqtt_message which holds info for publishing an mqtt message.
 *
 * @param[out] out_mqtt_message A pointer to a #pnp_mqtt_message instance to initialize.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK #pnp_mqtt_message is initialied successfully.
 * @retval #AZ_ERROR_ARG The pointer to the #pnp_mqtt_message instance is NULL.
 */
az_result pnp_mqtt_message_init(pnp_mqtt_message* out_mqtt_message);

/**
 * @brief Creates a request id #az_span for use in sending twin messages. Value increments on each
 * call.  Capable of holding a 10 digit number (base 10).
 *
 * @return An #az_span containing the request id.
 */
az_span pnp_mqtt_get_request_id(void);

#endif // PNP_MQTT_MESSAGE_H
