// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief MQTT 5 configuration options for abstraction layer and connection.
 *
 * @note  You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_CONFIG_H
#define _az_MQTT5_CONFIG_H

#ifdef AZ_MQTT5_USER_CONFIG
#include <az_mqtt5_user_config.h>
#endif // AZ_MQTT5_USER_CONFIG

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

#ifndef AZ_MQTT5_CONNECTION_MIN_RETRY_DELAY_MSEC
/**
 * @brief The minimum delay in milliseconds between retry attempts to connect to the MQTT 5 broker.
 */
#define AZ_MQTT5_CONNECTION_MIN_RETRY_DELAY_MSEC (1000)
#endif // AZ_MQTT5_CONNECTION_MIN_RETRY_DELAY_MSEC

#ifndef AZ_MQTT5_CONNECTION_MAX_RETRY_DELAY_MSEC
/**
 * @brief The maximum delay in milliseconds between retry attempts to connect to the MQTT 5 broker.
 */
#define AZ_MQTT5_CONNECTION_MAX_RETRY_DELAY_MSEC (100000)
#endif // AZ_MQTT5_CONNECTION_MAX_RETRY_DELAY_MSEC

#ifndef AZ_MQTT5_CONNECTION_MAX_RANDOM_JITTER_MSEC
/**
 * @brief The maximum delay in milliseconds to add to the delay between retry attempts to connect
 * to the MQTT 5 broker.
 */
#define AZ_MQTT5_CONNECTION_MAX_RANDOM_JITTER_MSEC (5000)
#endif // AZ_MQTT5_CONNECTION_MAX_RANDOM_JITTER_MSEC

#ifndef AZ_MQTT5_CONNECTION_MAX_CONNECT_ATTEMPTS
/**
 * @brief The maximum number of retry attempts to connect to the MQTT 5 broker. No maximum number of
 * retry attempts if set to -1.
 */
#define AZ_MQTT5_CONNECTION_MAX_CONNECT_ATTEMPTS (-1)
#endif // AZ_MQTT5_CONNECTION_MAX_CONNECT_ATTEMPTS

#ifndef AZ_MQTT5_CONNECTION_DISCONNECT_HANDSHAKE_TIMEOUT_MSEC
/**
 * @brief The timeout in milliseconds to wait for the MQTT broker to respond to a
 * disconnect request. If the handshake fails, the WILL message will be sent out by the broker.
 */
#define AZ_MQTT5_CONNECTION_DISCONNECT_HANDSHAKE_TIMEOUT_MSEC (5000)
#endif // AZ_MQTT5_CONNECTION_DISCONNECT_HANDSHAKE_TIMEOUT_MSEC

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_CONFIG_H
