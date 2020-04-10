// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_http_policy_logging_private.h"
#include "az_http_policy_private.h"
#include "az_span_private.h"
#include <az_http_internal.h>
#include <az_http_transport.h>
#include <az_log_internal.h>
#include <az_platform_internal.h>

#include <_az_cfg.h>

enum
{
  _az_LOG_LENGTHY_VALUE_MAX_LENGTH
  = 50, // When we print values, such as header values, if they are longer than
        // _az_LOG_VALUE_MAX_LENGTH, we trim their contents (decorate with ellipsis in the middle)
        // to make sure each individual header value does not exceed _az_LOG_VALUE_MAX_LENGTH so
        // that they don't blow up the logs.
};

static az_span _az_http_policy_logging_append_lengthy_value(az_span ref_log_msg, az_span value)
{
  // The caller should validate that ref_log_msg is large enough to contain the value az_span
  // This means, ref_log_msg must have at least _az_LOG_LENGTHY_VALUE_MAX_LENGTH (i.e. 50) bytes
  // available.
  AZ_PRECONDITION(
      (az_span_capacity(ref_log_msg) - az_span_length(ref_log_msg))
      >= _az_LOG_LENGTHY_VALUE_MAX_LENGTH);

  int32_t value_size = az_span_length(value);

  if (value_size <= _az_LOG_LENGTHY_VALUE_MAX_LENGTH)
  {
    return az_span_append(ref_log_msg, value);
  }

  az_span const ellipsis = AZ_SPAN_FROM_STR(" ... ");
  int32_t const ellipsis_len = az_span_length(ellipsis);

  int32_t const first
      = (_az_LOG_LENGTHY_VALUE_MAX_LENGTH / 2) - ((ellipsis_len / 2) + (ellipsis_len % 2)); // 22

  int32_t const last
      = ((_az_LOG_LENGTHY_VALUE_MAX_LENGTH / 2) + (_az_LOG_LENGTHY_VALUE_MAX_LENGTH % 2)) // 23
      - (ellipsis_len / 2);

  AZ_PRECONDITION((first + last + az_span_length(ellipsis)) == _az_LOG_LENGTHY_VALUE_MAX_LENGTH);

  ref_log_msg = az_span_append(ref_log_msg, az_span_slice(value, 0, first));
  ref_log_msg = az_span_append(ref_log_msg, ellipsis);
  return az_span_append(ref_log_msg, az_span_slice(value, value_size - last, value_size));
}

static az_result _az_http_policy_logging_append_http_request_msg(
    _az_http_request const* request,
    az_span* ref_log_msg)
{
  az_span http_request_string = AZ_SPAN_FROM_STR("HTTP Request : ");
  az_span null_string = AZ_SPAN_FROM_STR("NULL");

  int32_t required_length = az_span_length(http_request_string);
  if (request == NULL)
  {
    required_length += az_span_length(null_string);
  }
  else
  {
    required_length
        = az_span_length(request->_internal.method) + az_span_length(request->_internal.url) + 1;
  }

  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(*ref_log_msg, required_length);

  *ref_log_msg = az_span_append(*ref_log_msg, http_request_string);

  if (request == NULL)
  {
    *ref_log_msg = az_span_append(*ref_log_msg, null_string);
    return AZ_OK;
  }

  *ref_log_msg = az_span_append(*ref_log_msg, request->_internal.method);
  *ref_log_msg = az_span_append_uint8(*ref_log_msg, ' ');
  *ref_log_msg = az_span_append(*ref_log_msg, request->_internal.url);

  int32_t const headers_count = _az_http_request_headers_count(request);

  az_span new_line_tab_string = AZ_SPAN_FROM_STR("\n\t");
  az_span colon_separator_string = AZ_SPAN_FROM_STR(" : ");

  for (int32_t index = 0; index < headers_count; ++index)
  {
    az_pair header = { 0 };
    AZ_RETURN_IF_FAILED(az_http_request_get_header(request, index, &header));

    required_length = az_span_length(new_line_tab_string) + az_span_length(header.key);
    if (az_span_length(header.value) > 0)
    {
      required_length += _az_LOG_LENGTHY_VALUE_MAX_LENGTH + az_span_length(colon_separator_string);
    }

    AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(*ref_log_msg, required_length);

    *ref_log_msg = az_span_append(*ref_log_msg, new_line_tab_string);

    *ref_log_msg = az_span_append(*ref_log_msg, header.key);

    if (az_span_length(header.value) > 0)
    {
      *ref_log_msg = az_span_append(*ref_log_msg, colon_separator_string);
      *ref_log_msg = _az_http_policy_logging_append_lengthy_value(*ref_log_msg, header.value);
    }
  }

  return AZ_OK;
}

static az_result _az_http_policy_logging_append_http_response_msg(
    az_http_response* ref_response,
    int64_t duration_msec,
    _az_http_request const* request,
    az_span* ref_log_msg)
{
  az_span http_response_string = AZ_SPAN_FROM_STR("HTTP Response (");
  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(*ref_log_msg, az_span_length(http_response_string));
  *ref_log_msg = az_span_append(*ref_log_msg, http_response_string);

  AZ_RETURN_IF_FAILED(az_span_append_i64toa(*ref_log_msg, duration_msec, ref_log_msg));

  az_span ms_string = AZ_SPAN_FROM_STR("ms)");
  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(*ref_log_msg, az_span_length(ms_string));
  *ref_log_msg = az_span_append(*ref_log_msg, ms_string);

  if (ref_response == NULL || az_span_length(ref_response->_internal.http_response) == 0)
  {
    az_span is_empty_string = AZ_SPAN_FROM_STR(" is empty");
    AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(*ref_log_msg, az_span_length(is_empty_string));
    *ref_log_msg = az_span_append(*ref_log_msg, is_empty_string);
    return AZ_OK;
  }

  az_span colon_separator_string = AZ_SPAN_FROM_STR(" : ");
  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(*ref_log_msg, az_span_length(colon_separator_string));
  *ref_log_msg = az_span_append(*ref_log_msg, colon_separator_string);

  az_http_response_status_line status_line = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_get_status_line(ref_response, &status_line));
  AZ_RETURN_IF_FAILED(
      az_span_append_u64toa(*ref_log_msg, (uint64_t)status_line.status_code, ref_log_msg));

  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(*ref_log_msg, az_span_length(status_line.reason_phrase) + 1);
  *ref_log_msg = az_span_append_uint8(*ref_log_msg, ' ');
  *ref_log_msg = az_span_append(*ref_log_msg, status_line.reason_phrase);

  az_span new_line_tab_string = AZ_SPAN_FROM_STR("\n\t");

  for (az_pair header;
       az_http_response_get_next_header(ref_response, &header) != AZ_ERROR_ITEM_NOT_FOUND;)
  {
    int32_t required_length = az_span_length(new_line_tab_string) + az_span_length(header.key);
    if (az_span_length(header.value) > 0)
    {
      required_length += _az_LOG_LENGTHY_VALUE_MAX_LENGTH + az_span_length(colon_separator_string);
    }

    AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(*ref_log_msg, required_length);

    *ref_log_msg = az_span_append(*ref_log_msg, new_line_tab_string);
    *ref_log_msg = az_span_append(*ref_log_msg, header.key);

    if (az_span_length(header.value) > 0)
    {
      *ref_log_msg = az_span_append(*ref_log_msg, colon_separator_string);
      *ref_log_msg = _az_http_policy_logging_append_lengthy_value(*ref_log_msg, header.value);
    }
  }

  az_span new_lines_string = AZ_SPAN_FROM_STR("\n\n");
  az_span arrow_separator_string = AZ_SPAN_FROM_STR(" -> ");
  int32_t required_length
      = az_span_length(new_lines_string) + az_span_length(arrow_separator_string);
  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(*ref_log_msg, required_length);

  *ref_log_msg = az_span_append(*ref_log_msg, new_lines_string);
  *ref_log_msg = az_span_append(*ref_log_msg, arrow_separator_string);
  AZ_RETURN_IF_FAILED(_az_http_policy_logging_append_http_request_msg(request, ref_log_msg));

  return AZ_OK;
}

void _az_http_policy_logging_log_http_request(_az_http_request const* request)
{
  uint8_t log_msg_buf[AZ_LOG_MSG_BUF_SIZE] = { 0 };
  az_span log_msg = AZ_SPAN_FROM_BUFFER(log_msg_buf);

  (void)_az_http_policy_logging_append_http_request_msg(request, &log_msg);

  az_log_write(AZ_LOG_HTTP_REQUEST, log_msg);
}

void _az_http_policy_logging_log_http_response(
    az_http_response const* response,
    int64_t duration_msec,
    _az_http_request const* request)
{
  uint8_t log_msg_buf[AZ_LOG_MSG_BUF_SIZE] = { 0 };
  az_span log_msg = AZ_SPAN_FROM_BUFFER(log_msg_buf);

  az_http_response response_copy = *response;

  (void)_az_http_policy_logging_append_http_response_msg(
      &response_copy, duration_msec, request, &log_msg);

  az_log_write(AZ_LOG_HTTP_RESPONSE, log_msg);
}

AZ_NODISCARD az_result az_http_pipeline_policy_logging(
    _az_http_policy* p_policies,
    void* p_data,
    _az_http_request* p_request,
    az_http_response* p_response)
{
  (void)p_data;

  if (az_log_should_write(AZ_LOG_HTTP_REQUEST))
  {
    _az_http_policy_logging_log_http_request(p_request);
  }

  if (!az_log_should_write(AZ_LOG_HTTP_RESPONSE))
  {
    // If no logging is needed, do not even measure the response time.
    return az_http_pipeline_nextpolicy(p_policies, p_request, p_response);
  }

  int64_t const start = az_platform_clock_msec();
  az_result const result = az_http_pipeline_nextpolicy(p_policies, p_request, p_response);
  int64_t const end = az_platform_clock_msec();

  _az_http_policy_logging_log_http_response(p_response, end - start, p_request);

  return result;
}
