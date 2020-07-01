// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

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
