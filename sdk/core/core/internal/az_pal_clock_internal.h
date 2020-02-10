// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PAL_CLOCK_INTERNAL_H
#define _az_PAL_CLOCK_INTERNAL_H

#include <stdint.h>
#include <time.h>

#include <_az_cfg_prefix.h>

AZ_INLINE int64_t _az_pal_clock_msec() {
  // Convert clock_t to milliseconds
  // Floating point arithmetic is used to cover CLOCKS_PER_SEC all values
  // including 12000, 2000, 1500, 300, 500, 100, 2
  return (int64_t)(clock() / (CLOCKS_PER_SEC / 1000.0));
}

#include <_az_cfg_suffix.h>

#endif
