// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_CLOCK_INTERNAL_H
#define _az_CLOCK_INTERNAL_H

#include <az_time_internal.h>

#include <stdint.h>
#include <time.h>

#include <_az_cfg_prefix.h>

AZ_INLINE uint64_t _az_clock_msec() {
  // Convert clock_t to milliseconds
  // Floating point arithmetic is used to cover CLOCKS_PER_SEC all values
  // including 12000, 2000, 1500, 300, 500, 100, 2
  return (uint64_t)(clock() / (CLOCKS_PER_SEC / (double)_az_TIME_MILLISECONDS_PER_SECOND));
}

#include <_az_cfg_suffix.h>

#endif
