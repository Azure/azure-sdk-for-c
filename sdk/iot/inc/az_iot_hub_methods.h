// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>
#include <az_result.h>
#include "az_iot_mqtt.h"
#include "az_iot_hub_client.h"

#ifndef _az_IOT_METHODS_H
#define _az_IOT_METHODS_H

#include <_az_cfg_prefix.h>

az_result az_iot_hub_methods_get_subscribe_topic(az_iot_hub_client* client, az_iot_topic* mqtt_topic_filter);

typedef struct az_iot_method_request {
    az_span request_id; // NULL terminated
    az_span name; // NULL terminated
    uint8_t status;
    az_span payload;
    az_span response_timeout; // NULL terminated
    az_span connection_timeout; // NULL terminated
} az_iot_method_response;

typedef struct az_iot_method_response {
    uint8_t status;
    az_span request_id; // NULL terminated
    az_span payload;
} az_iot_method_response;

az_result az_iot_methods_send_response(az_iot_hub_client* client, az_iot_method_response* response, az_iot_mqtt_publish *mqtt_pub);

az_result az_iot_methods_handle(az_iot_hub_client* client, az_iot_mqtt_publish* pub_received, az_iot_method_request* method_request);

az_result az_iot_hub_methods_send_response(az_iot_hub_client* client, az_iot_mqtt_publish* pub_received, az_iot_method_response* method_request);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_METHODS_H
