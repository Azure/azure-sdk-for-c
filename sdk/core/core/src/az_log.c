// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_log_private.h"
#include "az_span_private.h"
#include <az_http.h>
#include <az_http_transport.h>
#include <az_log.h>
#include <az_log_internal.h>
#include <az_span.h>
#include <az_config.h>

#include <stdbool.h>
#include <stddef.h>

#include <_az_cfg.h>

enum
{
  _az_LOG_VALUE_MAX_LENGTH
  = 50, // When we print values, such as header values, if they are longer than
        // _az_LOG_VALUE_MAX_LENGTH, we trim their contents (decorate with ellipsis in the middle)
        // to make sure each individual header value does not exceed _az_LOG_VALUE_MAX_LENGTH so
        // that they don't blow up the logs.
};

static az_log_classification const* _az_log_classifications = NULL;
static size_t _az_log_classifications_length = 0;
static az_log_fn _az_log_listener = NULL;

void az_log_set_classifications(
    az_log_classification const* classifications,
    size_t classifications_length)
{
  // TODO: thread safety
  _az_log_classifications = classifications;
  _az_log_classifications_length = classifications_length;
}

void az_log_set_listener(az_log_fn listener)
{
  // TODO: thread safety
  _az_log_listener = listener;
}

void az_log_write(az_log_classification classification, az_span message)
{
  // TODO: thread safety
  if (_az_log_listener != NULL && az_log_should_write(classification))
  {
    (*_az_log_listener)(classification, message);
  }
}

bool az_log_should_write(az_log_classification classification)
{
  // TODO: thread safety
  if (_az_log_listener == NULL)
  {
    // If no one is listening, don't attempt to log.
    return false;
  }
  if (_az_log_classifications == NULL || _az_log_classifications_length == 0)
  {
    // If the user hasn't registered any classifications, then we log everything.
    return true;
  }

  for (size_t i = 0; i < _az_log_classifications_length; ++i)
  {
    // Return true if a classification is in the customer-provided whitelist.
    if (_az_log_classifications[i] == classification)
    {
      return true;
    }
  }

  // Classification is not in the whitelist - return false.
  return false;
}

static az_result _az_log_value_msg(az_span* log_msg_bldr, az_span value)
{
  int32_t value_size = az_span_length(value);

  if (value_size <= _az_LOG_VALUE_MAX_LENGTH)
  {
    AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, value, log_msg_bldr));
    return AZ_OK;
  }

  az_span const ellipsis = AZ_SPAN_FROM_STR(" ... ");
  int32_t const ellipsis_len = az_span_length(ellipsis);

  int32_t const first = (_az_LOG_VALUE_MAX_LENGTH / 2) - ((ellipsis_len / 2) + (ellipsis_len % 2));

  int32_t const last
      = ((_az_LOG_VALUE_MAX_LENGTH / 2) + (_az_LOG_VALUE_MAX_LENGTH % 2)) - (ellipsis_len / 2);

  AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, az_span_take(value, first), log_msg_bldr));

  AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, ellipsis, log_msg_bldr));

  AZ_RETURN_IF_FAILED(
      az_span_append(*log_msg_bldr, az_span_drop(value, value_size - last), log_msg_bldr));

  return AZ_OK;
}

static az_result _az_log_http_request_msg(az_span* log_msg_bldr, _az_http_request* hrb)
{
  AZ_RETURN_IF_FAILED(
      az_span_append(*log_msg_bldr, AZ_SPAN_FROM_STR("HTTP Request : "), log_msg_bldr));

  if (hrb == NULL)
  {
    AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, AZ_SPAN_FROM_STR("NULL"), log_msg_bldr));
    return AZ_OK;
  }

  AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, hrb->_internal.method, log_msg_bldr));

  AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, AZ_SPAN_FROM_STR(" "), log_msg_bldr));

  AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, hrb->_internal.url, log_msg_bldr));

  int32_t const headers_count = _az_http_request_headers_count(hrb);
  for (int32_t index = 0; index < headers_count; ++index)
  {
    AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, AZ_SPAN_FROM_STR("\n\t"), log_msg_bldr));

    az_pair header = { 0 };
    AZ_RETURN_IF_FAILED(az_http_request_get_header(hrb, index, &header));
    AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, header.key, log_msg_bldr));

    if (az_span_length(header.value) > 0)
    {
      AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, AZ_SPAN_FROM_STR(" : "), log_msg_bldr));
      AZ_RETURN_IF_FAILED(_az_log_value_msg(log_msg_bldr, header.value));
    }
  }

  return AZ_OK;
}

static az_result _az_log_http_response_msg(
    az_span* log_msg_bldr,
    az_http_response* response,
    int64_t duration_msec,
    _az_http_request* hrb)
{
  AZ_RETURN_IF_FAILED(
      az_span_append(*log_msg_bldr, AZ_SPAN_FROM_STR("HTTP Response ("), log_msg_bldr));

  AZ_RETURN_IF_FAILED(az_span_append_int64(log_msg_bldr, duration_msec));
  AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, AZ_SPAN_FROM_STR("ms) "), log_msg_bldr));

  if (response == NULL || az_span_length(response->_internal.http_response) == 0)
  {
    AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, AZ_SPAN_FROM_STR("is empty"), log_msg_bldr));
    return AZ_OK;
  }

  AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, AZ_SPAN_FROM_STR(": "), log_msg_bldr));

  az_http_response_status_line status_line = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_get_status_line(response, &status_line));

  AZ_RETURN_IF_FAILED(az_span_append_uint64(log_msg_bldr, (uint64_t)status_line.status_code));

  AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, AZ_SPAN_FROM_STR(" "), log_msg_bldr));
  AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, status_line.reason_phrase, log_msg_bldr));

  for (az_pair header;
       az_http_response_get_next_header(response, &header) != AZ_ERROR_ITEM_NOT_FOUND;)
  {
    AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, AZ_SPAN_FROM_STR("\n\t"), log_msg_bldr));
    AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, header.key, log_msg_bldr));

    if (az_span_length(header.value) > 0)
    {
      AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, AZ_SPAN_FROM_STR(" : "), log_msg_bldr));
      AZ_RETURN_IF_FAILED(_az_log_value_msg(log_msg_bldr, header.value));
    }
  }

  AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, AZ_SPAN_FROM_STR("\n\n"), log_msg_bldr));
  AZ_RETURN_IF_FAILED(az_span_append(*log_msg_bldr, AZ_SPAN_FROM_STR(" -> "), log_msg_bldr));
  AZ_RETURN_IF_FAILED(_az_log_http_request_msg(log_msg_bldr, hrb));

  return AZ_OK;
}

void _az_log_http_request(_az_http_request* hrb)
{
  uint8_t log_msg_buf[AZ_LOG_MSG_BUF_SIZE] = { 0 };
  az_span log_msg_bldr = AZ_SPAN_FROM_BUFFER(log_msg_buf);
  (void)_az_log_http_request_msg(&log_msg_bldr, hrb);
  az_log_write(AZ_LOG_HTTP_REQUEST, log_msg_bldr);
}

void _az_log_http_response(az_http_response* response, int64_t duration_msec, _az_http_request* hrb)
{
  uint8_t log_msg_buf[AZ_LOG_MSG_BUF_SIZE] = { 0 };
  az_span log_msg_bldr = AZ_SPAN_FROM_BUFFER(log_msg_buf);
  (void)_az_log_http_response_msg(&log_msg_bldr, response, duration_msec, hrb);
  az_log_write(AZ_LOG_HTTP_RESPONSE, log_msg_bldr);
}
