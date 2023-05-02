// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Defines platform internals that are used across other platforms.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_PLATFORM_INTERNAL_H
#define _az_PLATFORM_INTERNAL_H

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Timer callback.
 *
 * @details Function may be re-entered by timer callback, locks must be used if re-entrancy is
 * not desired.
 *
 * @param[in] callback_context Data passed by the SDK during the #az_platform_timer_create call.
 */
typedef void (*_az_platform_timer_callback)(void* callback_context);

/**
 * @brief Common timer struct. Contains pointer to callback and data pointer.
 */
typedef struct
{
  struct
  {
    _az_platform_timer_callback callback;
    void* callback_context;
  } _internal;
} _az_platform_timer_common;

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_PLATFORM_INTERNAL_H
