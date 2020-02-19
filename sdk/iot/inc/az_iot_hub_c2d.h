// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>
#include <az_result.h>
#include "az_iot_mqtt.h"
#include "az_iot_hub_client.h"
#include "az_iot_hub_properties.h"

#ifndef _az_IOT_HUB_C2D_H
#define _az_IOT_HUB_C2D_H

#include <_az_cfg_prefix.h>

az_result az_iot_hub_c2d_get_subscribe_topic(az_iot_hub_client* client, az_iot_topic* mqtt_topic_filter);

typedef struct az_iot_hub_c2d_request {
    az_span payload;
    az_iot_hub_properties properties;
} az_iot_hub_c2d_request;

az_result az_iot_c2d_handle(az_iot_hub_client* client, az_iot_mqtt_publish* pub_received, az_iot_hub_c2d_request* request);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_HUB_C2D_H
