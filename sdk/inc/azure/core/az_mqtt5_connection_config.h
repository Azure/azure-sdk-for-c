/**
 * @file
 *
 * @brief MQTT 5 connection configuration options.
 *
 * @note  You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_CONNECTION_CONFIG_H
#define _az_MQTT5_CONNECTION_CONFIG_H

#include <azure/core/az_config.h>

#ifndef AZ_MQTT5_CONNECTION_DEFAULT_MIN_RETRY_DELAY_MSEC
/**
 * @brief The default minimum retry delay in milliseconds.
 */
#define AZ_MQTT5_CONNECTION_DEFAULT_MIN_RETRY_DELAY_MSEC (1000)
#endif // AZ_MQTT5_CONNECTION_DEFAULT_MIN_RETRY_DELAY_MSEC

#ifndef AZ_MQTT5_CONNECTION_DEFAULT_MAX_RETRY_DELAY_MSEC
/**
 * @brief The default maximum retry delay in milliseconds.
 */
#define AZ_MQTT5_CONNECTION_DEFAULT_MAX_RETRY_DELAY_MSEC (100000)
#endif // AZ_MQTT5_CONNECTION_DEFAULT_MAX_RETRY_DELAY_MSEC

#ifndef AZ_MQTT5_CONNECTION_DEFAULT_MAX_RANDOM_JITTER_MSEC
/**
 * @brief The default maximum random jitter in milliseconds.
 */
#define AZ_MQTT5_CONNECTION_DEFAULT_MAX_RANDOM_JITTER_MSEC (5000)
#endif // AZ_MQTT5_CONNECTION_DEFAULT_MAX_RANDOM_JITTER_MSEC

#ifndef AZ_MQTT5_CONNECTION_DEFAULT_MAX_CONNECT_ATTEMPTS
/**
 * @brief The default maximum number of connect attempts.
 */
#define AZ_MQTT5_CONNECTION_DEFAULT_MAX_CONNECT_ATTEMPTS (-1)
#endif // AZ_MQTT5_CONNECTION_DEFAULT_MAX_CONNECT_ATTEMPTS

#ifndef AZ_MQTT5_CONNECTION_DEFAULT_DISCONNECT_HANDSHAKE_TIMEOUT_MSEC
/**
 * @brief The default timeout in milliseconds to wait for the MQTT broker to respond to a
 * disconnect request. If the handshake fails, the WILL message will be sent out by the broker.
 */
#define AZ_MQTT5_CONNECTION_DEFAULT_DISCONNECT_HANDSHAKE_TIMEOUT_MSEC (5000)
#endif // AZ_MQTT5_CONNECTION_DEFAULT_DISCONNECT_HANDSHAKE_TIMEOUT_MSEC

#endif // _az_MQTT5_CONNECTION_CONFIG_H
