// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Defines MQTT constructs for Mosquitto.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT_MOSQUITTO_H
#define _az_MQTT_MOSQUITTO_H

#include "mosquitto.h"
#include <azure/core/internal/az_mqtt_internal.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct
{
  az_mqtt_options_common platform_options;
  /**
   * The CA Trusted Roots span interpretable by the underlying MQTT implementation.
   */
  az_span certificate_authority_trusted_roots;
  az_span openssl_engine;
  struct mosquitto* mosquitto_handle;
} az_mqtt_options;

struct az_mqtt
{
  struct
  {
    az_mqtt_common platform_mqtt;
    az_mqtt_options options;
  } _internal;

  struct mosquitto* mosquitto_handle;
};

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT_MOSQUITTO_H
