// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>
#include <az_result.h>
#include "az_iot_mqtt.h"
#include "az_iot_provisioning_client.h"

#ifndef _az_IOT_PROVISIONING_REGISTER_H
#define _az_IOT_PROVISIONING_REGISTER_H

#include <_az_cfg_prefix.h>

az_result az_provisioning_register_get_subscribe_topic(az_provisioning_client* client, az_iot_topic* mqtt_topic_filter);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_PROVISIONING_REGISTER_H