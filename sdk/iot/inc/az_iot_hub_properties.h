// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>
#include <az_result.h>

#ifndef _az_IOT_HUB_PROPERTIES_H
#define _az_IOT_HUB_PROPERTIES_H

#include <_az_cfg_prefix.h>

typedef struct az_iot_hub_properties {
    struct {
        az_span property_string;   // "URIENCODED(name1=val1&name2=val2&...)"
    } _internal;
} az_iot_hub_properties;

// TODO: Do we need an enumerator (requires state preserved)

az_result az_iot_hub_properties_add(az_iot_hub_properties *properties, az_span name, az_span value);
az_result az_iot_hub_properties_read(az_iot_hub_properties *properties, az_span name, az_span value);
az_result az_iot_hub_properties_update(az_iot_hub_properties *properties, az_span name, az_span value);
az_result az_iot_hub_properties_remove(az_iot_hub_properties *properties, az_span name);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_HUB_PROPERTIES_H
