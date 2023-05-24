// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief MQTT configuration options.
 *
 * @note  You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT_CONFIG_H
#define _az_MQTT_CONFIG_H

#include <azure/core/_az_cfg_prefix.h>

#ifndef AZ_MQTT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS
#define AZ_MQTT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS (240)
#endif // AZ_MQTT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT_CONFIG_H
