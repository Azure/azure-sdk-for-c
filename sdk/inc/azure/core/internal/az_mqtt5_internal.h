// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Defines MQTT 5 platform internals used across other MQTT 5 implementations.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_INTERNAL_H
#define _az_MQTT5_INTERNAL_H

#include <azure/core/az_event.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_event_pipeline_internal.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief MQTT 5 Library handle (type defined by implementation)
 */
typedef struct az_mqtt5 az_mqtt5;

/**
 * @brief MQTT 5 common options that span across implementations.
 */
typedef struct
{
  bool clean_session;
} az_mqtt5_options_common;

/**
 * @brief Common mqtt 5 struct.
 */
typedef struct
{
  _az_event_pipeline* pipeline;
} az_mqtt5_common;

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_INTERNAL_H
