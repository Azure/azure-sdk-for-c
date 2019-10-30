// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_RESPONSE_PARSER_H
#define AZ_HTTP_RESPONSE_PARSER_H

#include <az_pair.h>
#include <az_span.h>
#include <az_span_reader.h>

#include <_az_cfg_prefix.h>

typedef enum {
  AZ_HTTP_STATUS_CODE_OK = 200,
} az_http_status_code;

/**
 * An HTTP response status line
 *
 * See https://tools.ietf.org/html/rfc7230#section-3.1.2
 */
typedef struct {
  uint8_t major_version;
  uint8_t minor_version;
  az_http_status_code status_code;
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
 * An HTTP response parser.
 */
typedef struct {
  az_span_reader reader;
  az_http_response_kind kind;
} az_http_response_parser;

/**
 * Initializes an HTTP response parser.
 */
AZ_NODISCARD az_result
az_http_response_parser_init(az_span const buffer, az_http_response_parser * const out);

/**
 * An HTTP status line.
 */
AZ_NODISCARD az_result az_http_response_parser_read_status_line(
    az_http_response_parser * const self,
    az_http_response_status_line * const out);

/**
 * An HTTP header.
 */
AZ_NODISCARD az_result az_http_response_parser_read_header(
    az_http_response_parser * const self,
    az_http_response_header * const out);

/**
 * An HTTP body.
 */
AZ_NODISCARD az_result az_http_response_parser_read_body(
    az_http_response_parser * const self,
    az_http_response_body * const out);

#include <_az_cfg_suffix.h>

#endif
