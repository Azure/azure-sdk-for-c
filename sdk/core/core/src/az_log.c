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

enum {
  _az_LOG_VALUE_MAX_LENGTH
  = 50, // When we print values, such as header vaules, if they are longer than
        // _az_LOG_VALUE_MAX_LENGTH, we trim their contents (decorate with ellipsis in the middle)
        // to make sure each individual header value does not exceed _az_LOG_VALUE_MAX_LENGTH so
        // that they don't blow up the logs.
  _az_LOG_MSG_BUF_SIZE = 1024, // Size (in bytes) of the buffer to allocate on stack when building a
                               // log message => the maximum size of the log message.
};

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
  if (_az_log_listener == NULL) {
    // If no one is listening, don't attempt to log.
    return false;
  }
  if (_az_log_classifications == NULL || _az_log_classifications_length == 0) {
    // If the user hasn't registered any classifications, then we log everything.
    return true;
  }

  for (size_t i = 0; i < _az_log_classifications_length; ++i) {
    // Return true if a classification is in the customer-provided whitelist.
    if (_az_log_classifications[i] == classification) {
      return true;
    }
  }

  // Classification is not in the whitelist - return false.
  return false;
}

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

static az_result _az_log_http_request_msg(
    az_span_builder * const log_msg_bldr,
    az_http_request_builder const * const hrb,
    uint8_t const indent) {
  for (uint8_t ntabs = 0; ntabs < indent; ++ntabs) {
    AZ_RETURN_IF_FAILED(az_span_builder_append_byte(log_msg_bldr, '\t'));
  }

  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("HTTP Request : ")));
  if (hrb == NULL) {
    return az_span_builder_append(log_msg_bldr, AZ_STR("NULL"));
  }

  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, hrb->method_verb));

  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR(" ")));

  AZ_RETURN_IF_FAILED(
      az_span_builder_append(log_msg_bldr, az_span_builder_result(&hrb->url_builder)));

  uint16_t const headers_end = hrb->headers_end;
  for (uint16_t i = 0; i < headers_end; ++i) {
    AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("\n\t\t")));
    for (uint8_t ntabs = 0; ntabs < indent; ++ntabs) {
      AZ_RETURN_IF_FAILED(az_span_builder_append_byte(log_msg_bldr, '\t'));
    }

    az_pair header = { 0 };
    AZ_RETURN_IF_FAILED(az_http_request_builder_get_header(hrb, i, &header));
    AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, header.key));

    if (!az_span_is_empty(header.value)) {
      AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR(" : ")));
      AZ_RETURN_IF_FAILED(_az_log_value_msg(log_msg_bldr, header.value));
    }
  }

  return AZ_OK;
}

static az_result _az_log_http_response_msg(
    az_span_builder * const log_msg_bldr,
    az_http_response const * const response,
    uint64_t const duration_msec,
    az_http_request_builder const * const hrb) {
  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("HTTP Response (")));
  AZ_RETURN_IF_FAILED(az_span_builder_append_uint64(log_msg_bldr, duration_msec));
  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("ms) ")));

  if (response == NULL || response->builder.length == 0) {
    return az_span_builder_append(log_msg_bldr, AZ_STR("is empty"));
  } else {
    AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR(": ")));
  }

  az_http_response_parser parser = { 0 };
  AZ_RETURN_IF_FAILED(
      az_http_response_parser_init(&parser, az_span_builder_result(&response->builder)));

  az_http_response_status_line status_line = { 0 };
  AZ_RETURN_IF_FAILED(az_http_response_parser_read_status_line(&parser, &status_line));

  AZ_RETURN_IF_FAILED(
      az_span_builder_append_uint64(log_msg_bldr, (uint64_t)status_line.status_code));

  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR(" ")));
  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, status_line.reason_phrase));

  az_pair header = { 0 };
  if (az_http_response_parser_read_header(&parser, &header) == AZ_OK) {
    do {
      AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("\n\t\t")));
      AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, header.key));

      if (!az_span_is_empty(header.value)) {
        AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR(" : ")));
        AZ_RETURN_IF_FAILED(_az_log_value_msg(log_msg_bldr, header.value));
      }
    } while (az_http_response_parser_read_header(&parser, &header) == AZ_OK);
  }

  AZ_RETURN_IF_FAILED(az_span_builder_append(log_msg_bldr, AZ_STR("\n\n")));
  return _az_log_http_request_msg(log_msg_bldr, hrb, 1);
}

void _az_log_http_request(az_http_request_builder const * const hrb) {
  uint8_t log_msg_buf[_az_LOG_MSG_BUF_SIZE] = { 0 };

  az_span_builder log_msg_bldr
      = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(log_msg_buf));

  (void)_az_log_http_request_msg(&log_msg_bldr, hrb, 0);

  az_log_write(AZ_LOG_HTTP_REQUEST, az_span_builder_result(&log_msg_bldr));
}

void _az_log_http_response(
    az_http_response const * const response,
    uint64_t const duration_msec,
    az_http_request_builder const * const hrb) {
  uint8_t log_msg_buf[_az_LOG_MSG_BUF_SIZE] = { 0 };

  az_span_builder log_msg_bldr
      = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(log_msg_buf));

  (void)_az_log_http_response_msg(&log_msg_bldr, response, duration_msec, hrb);

  az_log_write(AZ_LOG_HTTP_RESPONSE, az_span_builder_result(&log_msg_bldr));
}
