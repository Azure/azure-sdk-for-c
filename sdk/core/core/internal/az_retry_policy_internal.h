// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_RETRY_POLICY_INTERNAL_H
#define _az_RETRY_POLICY_INTERNAL_H

#include <az_result.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

enum {
  _az_RETRY_POLICY_DEFAULT_RETRY_DELAY_MSEC = 4 * 1000, // 4 seconds
  _az_RETRY_POLICY_DEFAULT_MAX_RETRY_DELAY_MSEC = 2 * 60 * 1000, // 2 minutes
};

typedef struct {
  int16_t max_tries;
  int32_t retry_delay_msec;
  int32_t max_retry_delay_msec;
} az_retry_policy;

AZ_INLINE az_result az_retry_policy_init(az_retry_policy * self) {
  *self = (az_retry_policy){
    .max_tries = 4,
    .retry_delay_msec = _az_RETRY_POLICY_DEFAULT_RETRY_DELAY_MSEC,
    .max_retry_delay_msec = _az_RETRY_POLICY_DEFAULT_MAX_RETRY_DELAY_MSEC,
  };

  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif
