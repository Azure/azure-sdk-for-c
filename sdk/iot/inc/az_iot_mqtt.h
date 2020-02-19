// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>

#ifndef _az_IOT_MQTT_H
#define _az_IOT_MQTT_H

#include <_az_cfg_prefix.h>

typedef struct az_iot_mqtt_connect {
    az_span client_id;  // NULL terminated string.
    az_span user;       // NULL terminated string.
    az_span password;   // NULL terminated string (may be an empty string).
} az_iot_mqtt_connect;

typedef struct az_iot_topic {
    az_span name;       // NULL terminated string.
    uint8_t qos;
} az_iot_topic;

typedef struct az_iot_mqtt_publish {
    az_iot_topic topic;
    az_span payload;
} az_iot_mqtt_publish;

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_MQTT_H
