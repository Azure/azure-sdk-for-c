// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_POLICY_RETRY_OPTIONS_H
#define _az_HTTP_POLICY_RETRY_OPTIONS_H

#include <az_result.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

enum {
  _az_HTTP_POLICY_RETRY_OPTIONS_DEFAULT_RETRY_DELAY_MSEC = 4 * 1000, // 4 seconds
  _az_HTTP_POLICY_RETRY_OPTIONS_DEFAULT_MAX_RETRY_DELAY_MSEC = 2 * 60 * 1000, // 2 minutes
};

/**
 * @brief options for retry policy
 *
 * max_tries specifies the maximum number of attempts an operation will be tried before producing an
 * error (0=default). A value of zero means that you accept our default policy. A value of 1 means 1
 * try and no retries.
 *
 * retry_delay_msec specifies the amount of delay (im milliseconds) to use before retrying an
 * operation (0=default). The delay increases exponentially with each retry up to a maximum
 * specified by max_retry_delay_msec. If you specify 0, then you must also specify 0 for
 * max_retry_delay_msec. If you specify retry_delay_msec, then you must also specify
 * max_retry_delay_msec, and max_retry_delay_msec should be equal to or greater than
 * retry_delay_msec.
 *
 * max_retry_delay_msec specifies the maximum delay allowed before retrying an operation
 * (0=default). If you specify 0, then you must also specify 0 for retry_delay_msec.
 */
typedef struct {
  int16_t max_tries;
  int32_t retry_delay_msec;
  int32_t max_retry_delay_msec;
} az_http_policy_retry_options;

AZ_INLINE az_http_policy_retry_options az_http_policy_retry_options_create() {
  return (az_http_policy_retry_options){
    .max_tries = 4,
    .retry_delay_msec = _az_HTTP_POLICY_RETRY_OPTIONS_DEFAULT_RETRY_DELAY_MSEC,
    .max_retry_delay_msec = _az_HTTP_POLICY_RETRY_OPTIONS_DEFAULT_MAX_RETRY_DELAY_MSEC,
  };
}

#include <_az_cfg_suffix.h>

#endif
