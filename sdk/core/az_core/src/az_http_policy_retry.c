// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_http_policy_private.h"
#include "az_http_private.h"
#include <az_config.h>
#include <az_config_internal.h>
#include <az_http_internal.h>
#include <az_log_internal.h>
#include <az_platform_internal.h>
#include <az_retry_internal.h>
#include <az_span_internal.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <_az_cfg.h>

static az_http_status_code const _default_status_codes[] = {
  AZ_HTTP_STATUS_CODE_REQUEST_TIMEOUT,
  AZ_HTTP_STATUS_CODE_TOO_MANY_REQUESTS,
  AZ_HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR,
  AZ_HTTP_STATUS_CODE_BAD_GATEWAY,
  AZ_HTTP_STATUS_CODE_SERVICE_UNAVAILABLE,
  AZ_HTTP_STATUS_CODE_GATEWAY_TIMEOUT,
  AZ_HTTP_STATUS_CODE_NONE,
};

AZ_NODISCARD az_http_policy_retry_options _az_http_policy_retry_options_default()
{
  return (az_http_policy_retry_options){
    .max_retries = 4,
    .retry_delay_msec = 4 * _az_TIME_MILLISECONDS_PER_SECOND, // 4 seconds
    .max_retry_delay_msec
    = 2 * _az_TIME_SECONDS_PER_MINUTE * _az_TIME_MILLISECONDS_PER_SECOND, // 2 minutes
    .status_codes = _default_status_codes,
  };
}

// TODO: Add unit tests
AZ_INLINE az_result _az_http_policy_retry_append_http_retry_msg(
    int16_t attempt,
    int32_t delay_msec,
    az_span* ref_log_msg)
{
  az_span retry_count_string = AZ_SPAN_FROM_STR("HTTP Retry attempt #");
  AZ_RETURN_IF_NOT_ENOUGH_SIZE(*ref_log_msg, az_span_size(retry_count_string));
  az_span remainder = az_span_copy(*ref_log_msg, retry_count_string);

  AZ_RETURN_IF_FAILED(az_span_i32toa(remainder, (int32_t)attempt, &remainder));

  az_span infix_string = AZ_SPAN_FROM_STR(" will be made in ");
  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remainder, az_span_size(infix_string));
  remainder = az_span_copy(remainder, infix_string);

  AZ_RETURN_IF_FAILED(az_span_i32toa(remainder, delay_msec, &remainder));

  az_span suffix_string = AZ_SPAN_FROM_STR("ms.");
  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remainder, az_span_size(suffix_string));
  remainder = az_span_copy(remainder, suffix_string);

  *ref_log_msg = az_span_slice(*ref_log_msg, 0, _az_span_diff(remainder, *ref_log_msg));

  return AZ_OK;
}

AZ_INLINE void _az_http_policy_retry_log(int16_t attempt, int32_t delay_msec)
{
  uint8_t log_msg_buf[AZ_LOG_MSG_BUF_SIZE] = { 0 };
  az_span log_msg = AZ_SPAN_FROM_BUFFER(log_msg_buf);

  (void)_az_http_policy_retry_append_http_retry_msg(attempt, delay_msec, &log_msg);

  _az_LOG_WRITE(AZ_LOG_HTTP_RETRY, log_msg);
}

AZ_INLINE AZ_NODISCARD int32_t _az_uint32_span_to_int32(az_span span)
{
  uint32_t value = 0;
  if (az_succeeded(az_span_atou32(span, &value)))
  {
    return value < INT32_MAX ? (int32_t)value : INT32_MAX;
  }

  return -1;
}

AZ_INLINE AZ_NODISCARD az_result _az_http_policy_retry_get_retry_after(
    az_http_response* ref_response,
    az_http_status_code const* status_codes,
    bool* should_retry,
    int32_t* retry_after_msec)
{
  az_http_response_status_line status_line = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_get_status_line(ref_response, &status_line));
  az_http_status_code const response_code = status_line.status_code;

  for (; *status_codes != AZ_HTTP_STATUS_CODE_NONE; ++status_codes)
  {
    if (*status_codes != response_code)
    {
      continue;
    }

    // Try to get the value of retry-after header, if there's one.
    *should_retry = true;

    az_pair header = { 0 };
    while (az_http_response_get_next_header(ref_response, &header) == AZ_OK)
    {
      if (az_span_is_content_equal_ignoring_case(header.key, AZ_SPAN_FROM_STR("retry-after-ms"))
          || az_span_is_content_equal_ignoring_case(
              header.key, AZ_SPAN_FROM_STR("x-ms-retry-after-ms")))
      {
        // The value is in milliseconds.
        int32_t const msec = _az_uint32_span_to_int32(header.value);
        if (msec >= 0) // int32_t max == ~24 days
        {
          *retry_after_msec = msec;
          return AZ_OK;
        }
      }
      else if (az_span_is_content_equal_ignoring_case(header.key, AZ_SPAN_FROM_STR("Retry-After")))
      {
        // The vaule is either seconds or date.
        int32_t const seconds = _az_uint32_span_to_int32(header.value);
        if (seconds >= 0) // int32_t max == ~68 years
        {
          *retry_after_msec = (seconds <= (INT32_MAX / _az_TIME_MILLISECONDS_PER_SECOND))
              ? seconds * _az_TIME_MILLISECONDS_PER_SECOND
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

  *should_retry = false;
  *retry_after_msec = -1;
  return AZ_OK;
}

AZ_NODISCARD az_result az_http_pipeline_policy_retry(
    _az_http_policy* p_policies,
    void* p_data,
    _az_http_request* p_request,
    az_http_response* p_response)
{
  az_http_policy_retry_options const* const retry_options
      = (az_http_policy_retry_options const*)p_data;

  int16_t const max_retries = retry_options->max_retries;
  int32_t const retry_delay_msec = retry_options->retry_delay_msec;
  int32_t const max_retry_delay_msec = retry_options->max_retry_delay_msec;
  az_http_status_code const* const status_codes = retry_options->status_codes;

  AZ_RETURN_IF_FAILED(_az_http_request_mark_retry_headers_start(p_request));

  az_context* const context = p_request->_internal.context;

  bool const should_log = _az_LOG_SHOULD_WRITE(AZ_LOG_HTTP_RETRY);
  az_result result = AZ_OK;
  int16_t attempt = 1;
  while (true)
  {
    AZ_RETURN_IF_FAILED(az_http_response_init(p_response, p_response->_internal.http_response));
    AZ_RETURN_IF_FAILED(_az_http_request_remove_retry_headers(p_request));

    result = az_http_pipeline_nextpolicy(p_policies, p_request, p_response);

    // Even HTTP 429, or 502 are expected to be AZ_OK, so the failed result is not retriable.
    if (attempt > max_retries || az_failed(result))
    {
      return result;
    }

    int32_t retry_after_msec = -1;
    bool should_retry = false;
    az_http_response response_copy = *p_response;
    AZ_RETURN_IF_FAILED(_az_http_policy_retry_get_retry_after(
        &response_copy, status_codes, &should_retry, &retry_after_msec));

    if (!should_retry)
    {
      return result;
    }

    ++attempt;

    if (retry_after_msec < 0)
    { // there wasn't any kind of "retry-after" response header
      retry_after_msec = _az_retry_calc_delay(attempt, retry_delay_msec, max_retry_delay_msec);
    }

    if (should_log)
    {
      _az_http_policy_retry_log(attempt, retry_after_msec);
    }

    az_platform_sleep_msec(retry_after_msec);

    if (context != NULL && az_context_has_expired(context, az_platform_clock_msec()))
    {
      return AZ_ERROR_CANCELED;
    }
  }

  return result;
}
