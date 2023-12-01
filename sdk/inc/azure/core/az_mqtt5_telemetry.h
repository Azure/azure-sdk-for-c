// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @brief Common include for az_mqtt5_telemetry consumer and producer.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_TELEMETRY_H
#define _az_MQTT5_TELEMETRY_H

#include <azure/core/internal/az_mqtt5_topic_parser_internal.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief MQTT5 Telemetry default topic format.
 *
 */
#define AZ_MQTT5_TELEMETRY_DEFAULT_TOPIC_FORMAT                                                  \
  "services/" _az_MQTT5_TOPIC_PARSER_SERVICE_ID_TOKEN "/" _az_MQTT5_TOPIC_PARSER_SENDER_ID_TOKEN \
  "/telemetry"

/**
 * @brief The default timeout in seconds for subscribing/publishing.
 */
#define AZ_MQTT5_TELEMETRY_DEFAULT_TIMEOUT_SECONDS 10
/**
 * @brief The default QOS to use for subscribing/publishing.
 */
#ifndef AZ_MQTT5_DEFAULT_TELEMETRY_QOS
#define AZ_MQTT5_DEFAULT_TELEMETRY_QOS AZ_MQTT5_QOS_AT_LEAST_ONCE
#endif

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_TELEMETRY_H