// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_MQTT5_NOTRANSPORT_H
#define _az_MQTT5_NOTRANSPORT_H

#include <azure/core/internal/az_mqtt5_internal.h>

#include <azure/core/_az_cfg_prefix.h>

typedef void* az_mqtt5_options;

struct az_mqtt5
{
  struct
  {
    az_mqtt5_common platform_mqtt5;
    az_mqtt5_options options;
  } _internal;
};

typedef void az_mqtt5_property_bag;

typedef void az_mqtt5_property_string;

typedef void az_mqtt5_property_stringpair;

typedef void az_mqtt5_property_binarydata;

AZ_NODISCARD az_result
az_mqtt5_init(az_mqtt5* mqtt5, void* notransport_handle, az_mqtt5_options const* options);

AZ_NODISCARD az_result
az_mqtt5_property_bag_init(az_mqtt5_property_bag* property_bag, az_mqtt5* mqtt5, void* options);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_NOTRANSPORT_H
