// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_RESPONSE_READ_H
#define AZ_HTTP_RESPONSE_READ_H

#include <az_pair.h>
#include <az_span.h>
#include <az_span_reader.h>

#include <_az_cfg_prefix.h>

/**
 * An HTTP response status line
 *
 * See https://tools.ietf.org/html/rfc7230#section-3.1.2
 */
typedef struct {
  uint8_t major_version;
  uint8_t minor_version;
  uint16_t status_code;
  az_span reason_phrase;
} az_http_response_status_line;

typedef az_pair az_http_response_header;

typedef az_span az_http_response_body;

typedef enum {
  AZ_HTTP_RESPONSE_NONE = 0,
  AZ_HTTP_RESPONSE_STATUS_LINE = 1,
  AZ_HTTP_RESPONSE_HEADER = 2,
  AZ_HTTP_RESPONSE_BODY = 3,
} az_http_response_kind;

/**
 * An HTTP response value is either `status line`, `header`, or `body`.
 */
typedef struct {
  az_http_response_kind kind;
  union {
    az_http_response_status_line status_line;
    az_http_response_header header;
    az_http_response_body body;
  } data;
} az_http_response_value;

typedef struct {
  az_span_reader reader;
  az_http_response_kind kind;
} az_http_response_state;

/**
 * Creates an HTTP response parser.
 */
AZ_NODISCARD az_http_response_state az_http_response_state_create(az_span const buffer);

/**
 * Returns a next HTTP response value.
 */
AZ_NODISCARD az_result az_http_response_state_read(
    az_http_response_state * const self,
    az_http_response_value * const out);

#include <_az_cfg_suffix.h>

#endif
