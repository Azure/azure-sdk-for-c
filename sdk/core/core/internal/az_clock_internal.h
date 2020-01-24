// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_CLOCK_INTERNAL_H
#define _az_CLOCK_INTERNAL_H

#include <stdint.h>
#include <time.h>

#include <_az_cfg_prefix.h>

AZ_INLINE uint64_t _az_clock_msec() {
  return clock() / (CLOCKS_PER_SEC / 1000); // convert clock_t to milliseconds
}

#include <_az_cfg_suffix.h>

#endif
