// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#include "az_http_policy_retry_private.h"
#include "az_log_private.h"
#include <az_http_policy_internal.h>
#include <az_http_policy_retry_options.h>
#include <az_http_response_parser.h>
#include <az_log_internal.h>
#include <az_pal.h>
#include <az_result.h>
#include <az_span_builder.h>
#include <az_str.h>
#include <az_time_internal.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <_az_cfg.h>

enum {
  _az_HTTP_POLICY_RETRY_OPTIONS_DEFAULT_MAX_TRIES = 4,

  _az_HTTP_POLICY_RETRY_OPTIONS_DEFAULT_RETRY_DELAY_MSEC
  = 4 * _az_TIME_MILLISECONDS_PER_SECOND, // 4 seconds

  _az_HTTP_POLICY_RETRY_OPTIONS_DEFAULT_MAX_RETRY_DELAY_MSEC
  = 2 * _az_TIME_SECONDS_PER_MINUTE * _az_TIME_MILLISECONDS_PER_SECOND, // 2 minutes
};

az_http_policy_retry_options az_http_policy_retry_options_create() {
  return (az_http_policy_retry_options){
    .max_tries = _az_HTTP_POLICY_RETRY_OPTIONS_DEFAULT_MAX_TRIES,
    .retry_delay_msec = _az_HTTP_POLICY_RETRY_OPTIONS_DEFAULT_RETRY_DELAY_MSEC,
    .max_retry_delay_msec = _az_HTTP_POLICY_RETRY_OPTIONS_DEFAULT_MAX_RETRY_DELAY_MSEC,
  };
}

AZ_NODISCARD bool az_http_policy_retry_options_validate(
    az_http_policy_retry_options const * retry_options) {

  if (retry_options == NULL || retry_options->max_tries < 0) {
    return false;
  }

  if (retry_options->retry_delay_msec < 0 || retry_options->max_retry_delay_msec < 0) {
    return false;
  }

  if (retry_options->retry_delay_msec == 0) {
    return retry_options->max_retry_delay_msec == 0;
  }

  return (retry_options->retry_delay_msec <= retry_options->max_retry_delay_msec);
}

AZ_INLINE az_result _az_log_http_retry_msg(
    az_span_builder * const log_msg_bldr,
    int16_t const attempt,
    int32_t const delay_msec) {
  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("HTTP Retry attempt")));

  if (attempt >= 0) {
    AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR(" #")));
    AZ_RETURN_IF_FAILED(az_span_builder_append_uint64(log_msg_bldr, (uint64_t)attempt));
  }

  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR(" will be made")));

  if (delay_msec >= 0) {
    AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR(" in ")));
    AZ_RETURN_IF_FAILED(az_span_builder_append_uint64(log_msg_bldr, (uint64_t)delay_msec));
    AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("ms")));
  }

  return az_span_builder_append_byte(log_msg_bldr, '.');
}

AZ_INLINE void _az_log_http_retry(int16_t const attempt, int32_t const delay_msec) {
  uint8_t log_msg_buf[_az_LOG_MSG_BUF_SIZE] = { 0 };

  az_span_builder log_msg_bldr
      = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(log_msg_buf));

  (void)_az_log_http_retry_msg(&log_msg_bldr, attempt, delay_msec);

  az_log_write(AZ_LOG_HTTP_RETRY, az_span_builder_result(&log_msg_bldr));
}


AZ_NODISCARD AZ_INLINE az_result _az_http_policy_retry_get_retry_after(
    az_span response,
    bool * should_retry,
    int32_t * retry_after_msec) {
  az_http_response_parser response_parser = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_parser_init(&response_parser, response));

  az_http_response_status_line status_line = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_parser_read_status_line(&response_parser, &status_line));

  switch (status_line.status_code) {
    // Keep retrying on the HTTP response codes below, return immediately otherwise.
    case AZ_HTTP_STATUS_CODE_REQUEST_TIMEOUT:
    case AZ_HTTP_STATUS_CODE_TOO_MANY_REQUESTS:
    case AZ_HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR:
    case AZ_HTTP_STATUS_CODE_BAD_GATEWAY:
    case AZ_HTTP_STATUS_CODE_SERVICE_UNAVAILABLE:
    case AZ_HTTP_STATUS_CODE_GATEWAY_TIMEOUT: {
      // Try to get the value of retry-after header, if there's one.
      *should_retry = true;

      az_pair header = { 0 };
      while (az_http_response_parser_read_header(&response_parser, &header) == AZ_OK) {
        if (az_span_is_equal_ignoring_case(header.key, AZ_STR("retry-after-ms"))
            || az_span_is_equal_ignoring_case(header.key, AZ_STR("x-ms-retry-after-ms"))) {
          // The value is in milliseconds.
          uint64_t msec;
          if (az_succeeded(az_span_to_uint64(header.value, &msec))) {
            *retry_after_msec
                = msec < INT32_MAX ? (int32_t)msec : INT32_MAX; // int32_t max == ~24 days

            return AZ_OK;
          }
        } else if (az_span_is_equal_ignoring_case(header.key, AZ_STR("Retry-After"))) {
          // The vaule is either seconds or date.
          uint64_t seconds;
          if (az_succeeded(az_span_to_uint64(header.value, &seconds))) {
            *retry_after_msec = (seconds < (INT32_MAX / _az_TIME_MILLISECONDS_PER_SECOND))
                ? (int32_t)seconds * _az_TIME_MILLISECONDS_PER_SECOND
                : INT32_MAX;

            return AZ_OK;
          }
          // TODO: Other possible value is HTTP Date. For that, we'll need to parse date, get
          // current date, subtract one from another, get seconds. And the device should have a
          // sense of calendar clock.
        }
      }

      *retry_after_msec = -1;
      return AZ_OK;
    }
    default:
      // None of the retriable HTTP codes above
      *should_retry = false;
      *retry_after_msec = -1;
      return AZ_OK;
  }
}

AZ_NODISCARD AZ_INLINE int32_t _az_http_policy_retry_calc_delay(
    int16_t attempt,
    int32_t retry_delay_msec,
    int32_t max_retry_delay_msec) {
  int32_t const exponential_retry_after = (int32_t)(
      (double)retry_delay_msec
      * (double)(attempt <= 30 ? (1 << attempt) : INT32_MAX)); // scale exponentially
  // Multiply the above by "(((1.0 / RAND_MAX) * rand()) / 2 + 0.8)" to add jitter factor from
  // 0.8 to 1.3

  return exponential_retry_after > max_retry_delay_msec ? max_retry_delay_msec
                                                        : exponential_retry_after;
}

AZ_NODISCARD az_result
_az_http_policy_retry(az_http_policy_retry_options * options, az_http_policy_callback next_policy) {
  int16_t const max_tries = options->max_tries;
  int32_t const retry_delay_msec = options->retry_delay_msec;
  int32_t const max_retry_delay_msec = options->max_retry_delay_msec;

  az_http_pipeline_policy_func const next_http_policy = next_policy.func;
  az_http_policy * const p_policies = next_policy.params.p_policies;
  az_http_request_builder * const hrb = next_policy.params.hrb;
  az_http_response * const response = next_policy.params.response;

  bool const should_log = az_log_should_write(AZ_LOG_HTTP_RETRY);
  az_result result = AZ_OK;
  int16_t attempt = 1;
  while (true) {
    AZ_RETURN_IF_FAILED(az_http_response_reset(response));
    AZ_RETURN_IF_FAILED(az_http_request_builder_remove_retry_headers(hrb));
    result = next_http_policy(p_policies, hrb, response);

    // Even HTTP 429, or 502 are expected to be AZ_OK, so the failed result is not retriable.
    if (attempt > max_tries || az_failed(result) || response->builder.length == 0) {
      return result;
    }

    int32_t retry_after_msec = -1;
    bool should_retry = false;
    AZ_RETURN_IF_FAILED(_az_http_policy_retry_get_retry_after(
        az_span_builder_result(&response->builder), &should_retry, &retry_after_msec));

    if (!should_retry) {
      return result;
    }

    ++attempt;

    if (retry_after_msec < 0) { // there wasn't any kind of "retry-after" response header
      retry_after_msec
          = _az_http_policy_retry_calc_delay(attempt, retry_delay_msec, max_retry_delay_msec);
    }

    if (should_log) {
      _az_log_http_retry(attempt, retry_after_msec);
    }

    az_pal_sleep(retry_after_msec);
  }

  return result;
}
