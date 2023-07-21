// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief MQTT 5 configuration options.
 *
 * @note  You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_CONFIG_H
#define _az_MQTT5_CONFIG_H

#include <azure/core/_az_cfg_prefix.h>

#ifndef AZ_MQTT5_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS
/**
 * @brief The default MQTT 5 keep-alive interval in seconds.
 */
#define AZ_MQTT5_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS (240)
#endif // AZ_MQTT5_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS

#ifndef AZ_MQTT5_DEFAULT_CONNECT_PORT
/**
 * @brief The default MQTT 5 port.
 */
#define AZ_MQTT5_DEFAULT_CONNECT_PORT (8883)
#endif // AZ_MQTT5_DEFAULT_CONNECT_PORT

#ifndef MQTT5_CLIENT_CERTIFICATES_MAX
/**
 * @brief The maximum number of certificates that can be used by the MQTT 5 client.
 */
#define MQTT5_CLIENT_CERTIFICATES_MAX (1)
#endif // MQTT5_CLIENT_CERTIFICATES_MAX

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_CONFIG_H
