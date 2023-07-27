// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Defines MQTT 5 constructs for Mosquitto.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_MOSQUITTO_H
#define _az_MQTT5_MOSQUITTO_H

#include "mosquitto.h"
#include <azure/core/internal/az_mqtt5_internal.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief MQTT 5 options for Mosquitto.
 *
 */
typedef struct
{
  /**
   * @brief Platform options that are common across all MQTT 5 implementations.
   */
  az_mqtt5_options_common platform_options;

  /**
   * @brief The CA Trusted Roots span interpretable by the underlying MQTT 5 implementation.
   *
   * @details We recommend avoiding configuring this option and instead using the default
   * OpenSSL trusted roots instead.
   */
  az_span certificate_authority_trusted_roots;

  /**
   * @brief OpenSSL engine to use for the underlying MQTT 5 implementation.
   */
  az_span openssl_engine;

  /**
   * @brief Handle to the underlying MQTT 5 implementation (Mosquitto).
   */
  struct mosquitto* mosquitto_handle;

  /**
   * @brief Whether to use TLS for the underlying MQTT 5 implementation.
   */
  bool disable_tls;
} az_mqtt5_options;

/**
 * @brief MQTT 5 client for Mosquitto.
 */
struct az_mqtt5
{
  struct
  {
    /**
     * @brief Handle to the underlying MQTT 5 implementation (Mosquitto).
     */
    struct mosquitto* mosquitto_handle;

    /**
     * @brief Platform MQTT 5 client that is common across all MQTT 5 implementations.
     */
    az_mqtt5_common platform_mqtt5;

    /**
     * @brief MQTT 5 options for Mosquitto.
     */
    az_mqtt5_options options;
  } _internal;
};

/**
 * @brief MQTT 5 property bag options for Mosquitto.
 *
 * @details This struct defines the MQTT 5 property bag options for Mosquitto. The property bag
 * is used to store MQTT 5 properties that can be sent with MQTT 5 messages.
 */
typedef struct
{
  /**
   * @brief Mosquitto specific MQTT 5 properties.
   */
  mosquitto_property* properties;
} az_mqtt5_property_bag_options;

/**
 * @brief MQTT 5 property bag.
 *
 */
typedef struct
{
  /**
   * @brief Mosquitto specific MQTT 5 properties.
   */
  mosquitto_property* properties;
} az_mqtt5_property_bag;

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_MOSQUITTO_H
