// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>
#include <az_result.h>
#include "az_iot_mqtt.h"
#include "az_iot_hub_client.h"

#ifndef _az_IOT_TWIN_H
#define _az_IOT_TWIN_H

#include <_az_cfg_prefix.h>

typedef struct az_iot_hub_twin_response {
    az_span payload;
    az_span request_id;
    az_span etag;
} az_iot_hub_twin_response;

az_result az_iot_twin_get(az_iot_hub_client* client, az_span request_id, az_iot_mqtt_publish *mqtt_pub);
az_result az_iot_twin_patch(az_iot_hub_client* client, az_span request_id, az_span etag, az_span, payload, az_iot_mqtt_pub *mqtt_pub);

// Handles both GET and PATCH:
az_result az_iot_methods_handle(az_iot_hub_client* client, az_iot_mqtt_publish* pub_received, az_iot_hub_twin_response* twin_response);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_TWIN_H
