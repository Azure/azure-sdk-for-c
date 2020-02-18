// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>
#include <az_result.h>
#include <time.h>
#include "az_iot_mqtt.h"
#include "az_iot_provisioning_client.h"

#ifndef _az_IOT_PROVISIONING_SAS_H
#define _az_IOT_PROVISIONING_SAS_H

#include <_az_cfg_prefix.h>

// Provisioning
typedef struct az_iot_provisioning_sas {
    struct {
        az_span audience; // URLENC("<id_scope>/registrations/<registration_id>"). This is HMAC256-ed together with expiration_time.
        az_span key_name; // Can be empty.
    } _internal;
} az_iot_provisioning_sas;

az_result az_iot_provisioning_sas_init(az_iot_provisioning_sas* sas, az_provisioning_client* client, az_span id_scope, az_span key_name);
az_result az_iot_provisioning_sas_update(az_iot_provisioning_sas* sas, time_t expiration_time, az_span base64_hmac256_signature, az_iot_mqtt_connect* mqtt_connect);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_PROVISIONING_SAS_H