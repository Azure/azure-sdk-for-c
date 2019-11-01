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
az_http_response_parser_init(az_http_response_parser * const out, az_span const buffer);

/**
 * An HTTP status line.
 */
AZ_NODISCARD az_result az_http_response_parser_get_status_line(
    az_http_response_parser * const self,
    az_http_response_status_line * const out);

/**
 * An HTTP header.
 */
AZ_NODISCARD az_result
az_http_response_parser_get_next_header(az_http_response_parser * const self, az_pair * const out);

/**
 * An HTTP body.
 */
AZ_NODISCARD az_result
az_http_response_parser_get_body(az_http_response_parser * const self, az_span * const out);

// Get information from HTTP response.

/**
 * Get an HTTP status line.
 */
AZ_NODISCARD az_result
az_http_response_get_status_line(az_span const self, az_http_response_status_line * const out);

/**
 * Get the next HTTP header.
 *
 * @p_header has to be either a previous header or an empty one for the first header.
 */
AZ_NODISCARD az_result
az_http_response_get_next_header(az_span const self, az_pair * const p_header);

/**
 * Get an HTTP body.
 *
 * @p_header has to be the last header!
 */
AZ_NODISCARD az_result
az_http_response_get_body(az_span const self, az_pair * const p_last_header, az_span * const body);

#include <_az_cfg_suffix.h>

#endif
