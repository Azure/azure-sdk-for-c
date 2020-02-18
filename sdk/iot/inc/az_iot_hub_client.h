// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>
#include "az_iot_mqtt.h"

#ifndef _az_IOT_HUB_CLIENT_H
#define _az_IOT_HUB_CLIENT_H

#include <_az_cfg_prefix.h>

typedef struct az_iot_hub_client {
    struct {
        az_span identity;   // deviceId[/moduleId]
    } _internal;
} az_provisioning_client;

void az_iot_hub_client_init(az_iot_hub_client* client, az_span device_id);
void az_iot_hub_client_init(az_iot_hub_client* client, az_span device_id, az_span module_id);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_HUB_CLIENT_H
