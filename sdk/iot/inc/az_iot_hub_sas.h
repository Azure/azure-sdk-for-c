// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>
#include <az_result.h>
#include <time.h>
#include "az_iot_mqtt.h"
#include "az_iot_hub_client.h"

#ifndef _az_IOT_SAS_H
#define _az_IOT_SAS_H

#include <_az_cfg_prefix.h>

typedef struct az_iot_hub_sas {
    struct {
        az_span audience;
        az_span key_name;
    } _internal;
} az_iot_hub_sas;

az_result az_iot_hub_sas_init(az_iot_hub_sas* sas, az_iot_hub_client* client, az_span hub_name, az_span key_name);
az_result az_iot_hub_sas_update(az_iot_hub_sas* sas, time_t expiration_time, az_span base64_hmac256_signature, az_iot_mqtt_connect* mqtt_connect);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_SAS_H
