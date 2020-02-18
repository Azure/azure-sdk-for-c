// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>
#include <az_result.h>
#include "az_iot_mqtt.h"

#ifndef _az_IOT_PROVISIONING_CLIENT_H
#define _az_IOT_PROVISIONING_CLIENT_H

#include <_az_cfg_prefix.h>

typedef struct az_provisioning_client {
    struct {
        az_span registration_id;
    } _internal;
} az_provisioning_client;

void az_provisioning_client_init(az_provisioning_client* client, az_span registration_id);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_PROVISIONING_CLIENT_H
