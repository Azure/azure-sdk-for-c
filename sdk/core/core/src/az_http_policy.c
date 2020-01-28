// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_log_private.h"
#include <az_clock_internal.h>
#include <az_http_client_internal.h>
#include <az_http_pipeline.h>
#include <az_http_policy.h>
#include <az_http_request_builder.h>
#include <az_http_response_parser.h>
#include <az_log.h>
#include <az_log_internal.h>
#include <az_mut_span.h>
#include <az_pal.h>
#include <az_retry_policy_internal.h>
#include <az_span.h>
#include <az_span_builder.h>
#include <az_str.h>
#include <az_url_internal.h>

#include <stddef.h>
#include <stdlib.h>

#include <_az_cfg.h>

AZ_NODISCARD AZ_INLINE az_result az_http_pipeline_nextpolicy(
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_http_response * const response) {

  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);

  // Transport Policy is the last policy in the pipeline
  //  it returns without calling nextpolicy
  if (p_policies[0].pfnc_process == NULL) {
    return AZ_ERROR_HTTP_PIPELINE_INVALID_POLICY;
  }

  return p_policies[0].pfnc_process(&(p_policies[1]), p_policies[0].data, hrb, response);
}

static az_span const AZ_MS_CLIENT_REQUESTID = AZ_CONST_STR("x-ms-client-request-id");

AZ_NODISCARD az_result az_http_pipeline_policy_uniquerequestid(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {
  (void)data;

  // TODO - add a UUID create implementation
  az_span const uniqueid = AZ_CONST_STR("123e4567-e89b-12d3-a456-426655440000");

  // Append the Unique GUID into the headers
  //  x-ms-client-request-id
  AZ_RETURN_IF_FAILED(az_http_request_builder_append_header(hrb, AZ_MS_CLIENT_REQUESTID, uniqueid));

  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_retry(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {
  // reset response to be written from the start
  if (data == NULL) {
    AZ_RETURN_IF_FAILED(az_http_response_reset(response));
    return az_http_pipeline_nextpolicy(p_policies, hrb, response);
  }

  az_retry_policy const * const retry_policy = (az_retry_policy const *)data;
  int16_t const max_tries = retry_policy->max_tries;

  bool const custom_retry_delay_limits = retry_policy->retry_delay_msec > 0
      && retry_policy->max_retry_delay_msec > 0
      && (retry_policy->retry_delay_msec <= retry_policy->max_retry_delay_msec);

  int32_t const retry_delay_msec = custom_retry_delay_limits
      ? retry_policy->retry_delay_msec
      : _az_RETRY_POLICY_DEFAULT_RETRY_DELAY_MSEC;

  int32_t const max_retry_delay_msec = custom_retry_delay_limits
      ? retry_policy->max_retry_delay_msec
      : _az_RETRY_POLICY_DEFAULT_MAX_RETRY_DELAY_MSEC;

  az_result result = AZ_OK;
  int16_t attempt = 1;
  while (true) {
    AZ_RETURN_IF_FAILED(az_http_response_reset(response));
    result = az_http_pipeline_nextpolicy(p_policies, hrb, response);

    // Even HTTP 429, or 502 are expected to be AZ_OK, so the failed result is not retriable.
    if (attempt > max_tries || az_failed(result) || response == NULL
        || response->builder.length == 0) {
      return result;
    }

    int32_t retry_after_msec = -1;
    {
      az_http_response_parser response_parser = { 0 };
      AZ_RETURN_IF_FAILED(az_http_response_parser_init(
          &response_parser, az_span_builder_result(&response->builder)));

      az_http_response_status_line status_line = { 0 };
      AZ_RETURN_IF_FAILED(az_http_response_parser_read_status_line(&response_parser, &status_line));

      switch (status_line.status_code) {
        // Keep retrying on the HTTP response codes below, return immediately otherwise.
        case 408:
        case 429:
        case 500:
        case 502:
        case 503:
        case 504: {
          // Try to get the value of retry-after header, if there's one.
          az_pair header = { 0 };
          while (az_http_response_parser_read_header(&response_parser, &header) == AZ_OK) {
            if (az_span_is_equal(header.key, AZ_STR("retry-after-ms"))
                || az_span_is_equal(header.key, AZ_STR("x-ms-retry-after-ms"))) {
              // The value is in milliseconds.
              uint64_t msec;
              if (az_succeeded(az_span_to_uint64(header.value, &msec))) {
                retry_after_msec
                    = msec < INT32_MAX ? (int32_t)msec : INT32_MAX; // int32_t max == ~24 days
                break;
              }
            } else if (az_span_is_equal(header.key, AZ_STR("Retry-After"))) {
              // The vaule is either seconds or date.
              uint64_t seconds;
              if (az_succeeded(az_span_to_uint64(header.value, &seconds))) {
                retry_after_msec = seconds < INT32_MAX / 1000 ? (int32_t)seconds * 1000 : INT32_MAX;
                break;
              }
              // TODO: Other possible value is HTTP Date. For that, we'll need to parse date, get
              // current date, subtract one from another, get seconds. And the device should have a
              // sense of calendar clock.
            }
          }
        } break;
        default:
          // None of the retriable HTTP codes above
          return result;
      }
    }

    ++attempt;

    if (retry_after_msec < 0) { // there wasn't any kind of "retry-after" response header
      int32_t const exponential_retry_after = (int32_t)(
          (double)retry_delay_msec
          * (double)(attempt <= 30 ? (1 << attempt) : INT32_MAX) // scale exponentially
          * (((1.0 / RAND_MAX) * rand()) / 2 + 0.8)); // add jitter factor from 0.8 to 1.3

      retry_after_msec = exponential_retry_after > max_retry_delay_msec ? max_retry_delay_msec
                                                                        : retry_after_msec;
    }

    if (az_log_should_write(AZ_LOG_HTTP_RETRY)) {
      _az_log_http_retry(attempt, retry_after_msec);
    }

    az_pal_wait(retry_after_msec);
  }

  return result;
}

typedef AZ_NODISCARD az_result (
    *_az_identity_auth_func)(void * const data, az_http_request_builder * const hrb);

typedef struct {
  _az_identity_auth_func _func;
} _az_identity_auth;

AZ_NODISCARD az_result az_http_pipeline_policy_authentication(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {
  AZ_CONTRACT_ARG_NOT_NULL(data);

  _az_identity_auth const * const auth = (_az_identity_auth const *)(data);
  AZ_CONTRACT_ARG_NOT_NULL(auth->_func);

  AZ_RETURN_IF_FAILED(auth->_func(data, hrb));

  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_logging(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {
  (void)data;
  if (az_log_should_write(AZ_LOG_HTTP_REQUEST)) {
    _az_log_http_request(hrb);
  }

  if (!az_log_should_write(AZ_LOG_HTTP_RESPONSE)) {
    // If no logging is needed, do not even measure the response time.
    return az_http_pipeline_nextpolicy(p_policies, hrb, response);
  }

  uint64_t const start = _az_clock_msec();
  az_result const result = az_http_pipeline_nextpolicy(p_policies, hrb, response);
  uint64_t const end = _az_clock_msec();

  _az_log_http_response(response, end - start, hrb);

  return result;
}

AZ_NODISCARD az_result az_http_pipeline_policy_bufferresponse(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {
  (void)data;

  // buffer response logic
  //  this might be uStream
  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_distributedtracing(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {
  (void)data;

  // Distributed tracing logic
  return az_http_pipeline_nextpolicy(p_policies, hrb, response);
}

AZ_NODISCARD az_result az_http_pipeline_policy_transport(
    az_http_policy * const p_policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response) {
  (void)data;

  // Transport policy is the last policy
  //  If a policy exists after the transport policy
  if (p_policies[0].pfnc_process != NULL) {
    return AZ_ERROR_HTTP_PIPELINE_INVALID_POLICY;
  }

  return az_http_client_send_request(hrb, response);
}
