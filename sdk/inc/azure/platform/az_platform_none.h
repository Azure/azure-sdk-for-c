// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Defines platform constructs when no platform is specified.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_PLATFORM_NONE_H
#define _az_PLATFORM_NONE_H

#include <azure/platform/internal/az_platform_internal.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct
{
  az_platform_timer_common platform_timer;
} az_platform_timer;

typedef void* az_platform_mutex;

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_PLATFORM_NONE_H
