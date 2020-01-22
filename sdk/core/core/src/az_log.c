// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_log_private.h"
#include <az_contract.h>
#include <az_http_response_parser.h>
#include <az_log.h>
#include <az_log_internal.h>
#include <az_span_builder.h>
#include <az_str.h>

#include <stdbool.h>
#include <stddef.h>

#include <_az_cfg.h>

static az_log_classification const * _az_log_classifications = NULL;
static size_t _az_log_classifications_length = 0;
static az_log _az_log_listener = NULL;

void az_log_set_classifications(
    az_log_classification const * const classifications,
    size_t const classifications_length) {
  // TODO: thread safety
  _az_log_classifications = classifications;
  _az_log_classifications_length = classifications_length;
}

void az_log_set_listener(az_log const listener) {
  // TODO: thread safety
  _az_log_listener = listener;
}

void az_log_write(az_log_classification const classification, az_span const message) {
  // TODO: thread safety
  if (_az_log_listener != NULL && az_log_should_write(classification)) {
    (*_az_log_listener)(classification, message);
  }
}

bool az_log_should_write(az_log_classification const classification) {
  // TODO: thread safety
  if (_az_log_classifications == NULL || _az_log_classifications_length == 0) {
    // If the user hasn't registered any classifications, then we log everything.
    return true;
  }

  for (size_t i = 0; i < _az_log_classifications_length; ++i) {
    if (_az_log_classifications[i] == classification) {
      return true;
    }
  }

  return false;
}

enum {
  _az_LOG_VALUE_MAX_LENGTH = 50,
  _az_LOG_MSG_BUF_SIZE = 1024,
};

static az_result _az_log_value_msg(az_span_builder * const log_msg_bldr, az_span const value) {
  size_t value_size = value.size;
  if (value_size <= _az_LOG_VALUE_MAX_LENGTH) {
    return az_span_builder_append(log_msg_bldr, value);
  } else {
    az_span const ellipsis = AZ_STR(" ... ");

    size_t const ellipsis_len = ellipsis.size;
    size_t const first = (_az_LOG_VALUE_MAX_LENGTH / 2) - ((ellipsis_len / 2) + (ellipsis_len % 2));

    size_t const last
        = ((_az_LOG_VALUE_MAX_LENGTH / 2) + (_az_LOG_VALUE_MAX_LENGTH % 2)) - (ellipsis_len / 2);

    AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, az_span_take(value, first)));

    AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, ellipsis));

    return az_span_builder_append(log_msg_bldr, az_span_drop(value, value_size - last));
  }
}

static az_result _az_log_hrb_msg(
    az_span_builder * const log_msg_bldr,
    az_http_request_builder const * const hrb) {
  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("HTTP Request : ")));
  if (hrb == NULL) {
    return az_span_builder_append(log_msg_bldr, AZ_STR("NULL"));
  }

  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("\n\tVerb: ")));
  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, hrb->method_verb));

  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("\n\tURL: ")));
  AZ_RETURN_IF_FAILED(
      az_span_builder_append(log_msg_bldr, az_span_builder_result(&hrb->url_builder)));

  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("\n\tHeaders: ")));

  uint16_t const headers_end = hrb->headers_end;
  if (headers_end == 0) {
    AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("none")));
  } else {
    uint16_t const retry_hdr = hrb->retry_headers_start;
    for (uint16_t i = 0; i < headers_end; ++i) {
      if (i == retry_hdr) {
        AZ_RETURN_IF_FAILED(
            az_span_builder_append(log_msg_bldr, AZ_STR("\n\t\t-- Retry Headers --")));
      }
      AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("\n\t\t")));

      az_pair header = { 0 };
      AZ_RETURN_IF_FAILED(az_http_request_builder_get_header(hrb, i, &header));

      AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, header.key));

      if (!az_span_is_empty(header.value)) {
        AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR(" : ")));
        AZ_RETURN_IF_FAILED(_az_log_value_msg(log_msg_bldr, header.value));
      }
    }
  }

  return AZ_OK;
}

static az_result _az_log_response_msg(
    az_span_builder * const log_msg_bldr,
    az_http_request_builder const * const hrb,
    az_http_response const * const response) {
  AZ_RETURN_IF_FAILED(_az_log_hrb_msg(log_msg_bldr, hrb));

  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("\n\nHTTP Response : ")));
  if (response == NULL) {
    return az_span_builder_append(log_msg_bldr, AZ_STR("NULL"));
  }

  az_http_response_parser parser = { 0 };
  AZ_RETURN_IF_FAILED(
      az_http_response_parser_init(&parser, az_span_builder_result(&response->builder)));

  az_http_response_status_line status_line = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_parser_read_status_line(&parser, &status_line));

  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("\n\tStatus Code: ")));
  AZ_RETURN_IF_FAILED(
      az_span_builder_append_uint64(log_msg_bldr, (uint64_t)status_line.status_code));

  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("\n\tStatus Line: ")));
  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, status_line.reason_phrase));

  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("\n\tHeaders: ")));
  az_pair header = { 0 };
  if (az_http_response_parser_read_header(&parser, &header) != AZ_OK) {
    AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("none")));
  } else {
    do {
      AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("\n\t\t")));
      AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, header.key));

      if (!az_span_is_empty(header.value)) {
        AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR(" : ")));
        AZ_RETURN_IF_FAILED(_az_log_value_msg(log_msg_bldr, header.value));
      }
    } while (az_http_response_parser_read_header(&parser, &header) == AZ_OK);
  }

  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("\n\tBody: ")));
  az_span body = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_parser_read_body(&parser, &body));
  if (az_span_is_empty(body)) {
    AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("none")));
  } else {
    AZ_RETURN_IF_FAILED(_az_log_value_msg(log_msg_bldr, body));
  }

  return AZ_OK;
}

AZ_INLINE az_result _az_log_slow_response_msg(
    az_span_builder * const log_msg_bldr,
    az_http_request_builder const * const hrb,
    az_http_response const * const response,
    uint32_t duration_msec) {
  AZ_RETURN_IF_FAILED(_az_log_response_msg(log_msg_bldr, hrb, response));
  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("\n\nResponse Duration: ")));
  AZ_RETURN_IF_FAILED(az_span_builder_append_uint64(log_msg_bldr, duration_msec));
  return az_span_builder_append(log_msg_bldr, AZ_STR("ms"));
}

AZ_INLINE az_result _az_log_error_msg(
    az_span_builder * const log_msg_bldr,
    az_http_request_builder const * const hrb,
    az_http_response const * const response,
    az_result result) {
  AZ_RETURN_IF_FAILED(_az_log_response_msg(log_msg_bldr, hrb, response));
  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("\n\nError: ")));
  return az_span_builder_append_uint64(log_msg_bldr, (uint32_t)result);
}

void _az_log_request(az_http_request_builder const * const hrb) {
  uint8_t log_msg_buf[_az_LOG_MSG_BUF_SIZE] = { 0 };

  az_span_builder log_msg_bldr
      = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(log_msg_buf));

  (void)_az_log_hrb_msg(&log_msg_bldr, hrb);

  az_log_write(AZ_LOG_REQUEST, az_span_builder_result(&log_msg_bldr));
}

void _az_log_response(
    az_http_request_builder const * const hrb,
    az_http_response const * const response) {
  uint8_t log_msg_buf[_az_LOG_MSG_BUF_SIZE] = { 0 };

  az_span_builder log_msg_bldr
      = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(log_msg_buf));

  (void)_az_log_response_msg(&log_msg_bldr, hrb, response);

  az_log_write(AZ_LOG_RESPONSE, az_span_builder_result(&log_msg_bldr));
}

void _az_log_slow_response(
    az_http_request_builder const * const hrb,
    az_http_response const * const response,
    uint32_t duration_msec) {
  uint8_t log_msg_buf[_az_LOG_MSG_BUF_SIZE] = { 0 };

  az_span_builder log_msg_bldr
      = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(log_msg_buf));

  (void)_az_log_slow_response_msg(&log_msg_bldr, hrb, response, duration_msec);

  az_log_write(AZ_LOG_SLOW_RESPONSE, az_span_builder_result(&log_msg_bldr));
}

void _az_log_error(
    az_http_request_builder const * const hrb,
    az_http_response const * const response,
    az_result result) {
  uint8_t log_msg_buf[_az_LOG_MSG_BUF_SIZE] = { 0 };

  az_span_builder log_msg_bldr
      = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(log_msg_buf));

  (void)_az_log_error_msg(&log_msg_bldr, hrb, response, result);

  az_log_write(AZ_LOG_ERROR, az_span_builder_result(&log_msg_bldr));
}
