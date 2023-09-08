// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Defines MQTT 5 constructs for Eclipse Paho Async.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_PAHOASYNC_H
#define _az_MQTT5_PAHOASYNC_H

#include "MQTTAsync.h"
#include <azure/core/internal/az_mqtt5_internal.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief MQTT 5 options for Paho Async.
 *
 */
typedef struct
{
  /**
   * @brief Platform options that are common across all MQTT 5 implementations.
   */
  az_mqtt5_options_common platform_options;

  /**
   * @brief The CA Trusted Roots path span interpretable by the underlying MQTT 5 implementation.
   *
   * @details We recommend avoiding configuring this option and instead using the default
   * OpenSSL trusted roots instead.
   */
  az_span certificate_authority_trusted_roots;

  /**
   * @brief Whether to use TLS for the underlying MQTT 5 implementation.
   */
  bool disable_tls;
} az_mqtt5_options;

struct az_mqtt5
{
  struct
  {
    /**
     * @brief Handle to the underlying MQTT 5 implementation (Paho Async).
     */
    MQTTAsync* pahoasync_handle;

    /**
     * @brief Platform MQTT 5 client that is common across all MQTT 5 implementations.
     */
    az_mqtt5_common platform_mqtt5;

    /**
     * @brief MQTT 5 options for Paho Async.
     */
    az_mqtt5_options options;
  } _internal;
};

/**
 * @brief MQTT5 property bag.
 *
 */
typedef struct
{
  /**
   * @brief Paho Async specific property array.
   *
   */
  MQTTProperties* pahoasync_properties;

  /**
   * @brief Paho Async property structure used for constructing the property array.
   * 
   * @note This should NOT be used as a stable reference for properties. It is only used for
   * constructing the property array.
   *
   */
  MQTTProperty pahoasync_property;
} az_mqtt5_property_bag;

/**
 * @brief MQTT 5 property string.
 *
 * @note String should always be freed after reading using #az_mqtt5_property_free_string.
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
 * @note String should always be freed after reading using #az_mqtt5_property_free_stringpair.
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
 * @note Binary data should always be freed after reading using #az_mqtt5_property_free_binarydata.
 */
typedef struct
{
  /**
   * @brief The binary data value of the property.
   */
  az_span bindata;
} az_mqtt5_property_binarydata;

/**
 * @brief Initializes the MQTT 5 instance specific to Eclipse Paho Async.
 *
 * @param mqtt5 The MQTT 5 instance.
 * @param pahoasync_handle The Paho Async handle, can't be NULL.
 * @param options The MQTT 5 options.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result
az_mqtt5_init(az_mqtt5* mqtt5, MQTTAsync* pahoasync_handle, az_mqtt5_options const* options);

/**
 * @brief Initializes an MQTT 5 property bag instance specific to Paho Async.
 *
 * @param property_bag The MQTT 5 property bag instance.
 * @param mqtt5 The MQTT 5 instance.
 * @param pahoasync_properties The MQTT 5 paho property structure.
 *
 * @note For certain MQTT stacks, the property bag will need to be associated with a particular MQTT
 * client handle.
 *
 * @note Application is responsible for freeing any allocated memory for the property bag.
 * Lifetime of the property bag is tied to the lifetime of the MQTT 5 request, a property bag
 * can be reused by resetting the property bag using #az_mqtt5_property_bag_clear. For Paho,
 * ensure that any allocated properties (if any) are freed before calling
 * #az_mqtt5_property_bag_clear.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_property_bag_init(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5* mqtt5,
    MQTTProperties* pahoasync_properties);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_PAHOASYNC_H
