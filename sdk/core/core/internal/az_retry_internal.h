// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_RETRY_INTERNAL_H
#define _az_RETRY_INTERNAL_H

#include <stdint.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD AZ_INLINE int32_t _az_retry_calc_delay(
    int16_t attempt,
    int32_t retry_delay_msec,
    int32_t max_retry_delay_msec)
{
  int32_t const exponential_retry_after
      = retry_delay_msec * (attempt <= 30 ? (1 << attempt) : INT32_MAX); // scale exponentially

  return exponential_retry_after > max_retry_delay_msec ? max_retry_delay_msec
                                                        : exponential_retry_after;
}

#include <_az_cfg_suffix.h>

#endif // _az_RETRY_INTERNAL_H
