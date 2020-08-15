// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Spinlock structure. Do not include this file directly. This file may get removed in the
 * future versions of the SDK.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_SPINLOCK_H
#define _az_SPINLOCK_H

#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct
{
  struct
  {
    volatile uintptr_t state;
  } _internal;
} _az_spinlock;

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_SPINLOCK_H
