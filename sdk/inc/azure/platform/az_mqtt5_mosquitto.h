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
    struct mosquitto** mosquitto_handle;

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
 * @brief MQTT 5 property bag.
 *
 */
typedef struct
{
  /**
   * @brief Mosquitto specific MQTT 5 properties.
   */
  mosquitto_property** mosq_properties;
} az_mqtt5_property_bag;

/**
 * @brief MQTT 5 property string.
 *
 * @note String should always be freed using #az_mqtt5_property_free_string.
 */
typedef struct
{
  /**
   * @brief The string value of the property.
   */
  az_span str;
} az_mqtt5_property_string;

/**
 * @brief MQTT 5 property string pair.
 *
 * @note String should always be freed using #az_mqtt5_property_free_stringpair.
 */
typedef struct
{
  /**
   * @brief The key of the property.
   */
  az_span key;

  /**
   * @brief The value of the property.
   */
  az_span value;
} az_mqtt5_property_stringpair;

/**
 * @brief MQTT 5 property binary data.
 *
 * @note Binary data should always be freed using #az_mqtt5_property_free_binarydata.
 */
typedef struct
{
  /**
   * @brief The binary data value of the property.
   */
  az_span bindata;
} az_mqtt5_property_binarydata;

/**
 * @brief Initializes the MQTT 5 instance specific to Eclipse Mosquitto.
 *
 * @param mqtt5 The MQTT 5 instance.
 * @param mosquitto_handle The Mosquitto handle, can't be NULL.
 * @param options The MQTT 5 options.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result
az_mqtt5_init(az_mqtt5* mqtt5, struct mosquitto** mosquitto_handle, az_mqtt5_options const* options);

/**
 * @brief Initializes an MQTT 5 property bag instance specific to Mosquitto.
 *
 * @param property_bag The MQTT 5 property bag instance.
 * @param mqtt5 The MQTT 5 instance.
 * @param mosq_properties The MQTT 5 mosquitto property structure.
 *
 * @note For certain MQTT stacks, the property bag will need to be associated with a particular MQTT
 * client handle.
 *
 * @note Application is responsible for freeing any allocated memory for the property bag.
 * Lifetime of the property bag is tied to the lifetime of the MQTT 5 request, a property bag
 * can be reused by resetting the property bag using #az_mqtt5_property_bag_clear.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_property_bag_init(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5* mqtt5,
    mosquitto_property** mosq_properties);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_MOSQUITTO_H
