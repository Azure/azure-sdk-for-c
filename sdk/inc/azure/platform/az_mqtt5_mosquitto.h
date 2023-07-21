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

/**
 * @brief MQTT options for Mosquitto.
 *
 */
typedef struct
{
  /**
   * @brief Platform options that are common across all MQTT implementations.
   */
  az_mqtt_options_common platform_options;

  /**
   * @brief The CA Trusted Roots span interpretable by the underlying MQTT implementation.
   *
   * @details We recommend avoiding configuring this option and instead using the default
   * OpenSSL trusted roots instead.
   */
  az_span certificate_authority_trusted_roots;

  /**
   * @brief OpenSSL engine to use for the underlying MQTT implementation.
   */
  az_span openssl_engine;

  /**
   * @brief Handle to the underlying MQTT implementation (Mosquitto).
   */
  struct mosquitto* mosquitto_handle;

  /**
   * @brief Whether to use TLS for the underlying MQTT implementation.
   */
  bool disable_tls;
} az_mqtt_options;

/**
 * @brief MQTT client for Mosquitto.
 */
struct az_mqtt
{
  struct
  {
    /**
     * @brief Handle to the underlying MQTT implementation (Mosquitto).
     */
    struct mosquitto* mosquitto_handle;

    /**
     * @brief Platform MQTT client that is common across all MQTT implementations.
     */
    az_mqtt_common platform_mqtt;

    /**
     * @brief MQTT options for Mosquitto.
     */
    az_mqtt_options options;
  } _internal;
};

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT_MOSQUITTO_H
