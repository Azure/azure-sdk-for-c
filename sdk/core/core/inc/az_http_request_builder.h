// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @brief Interface declaration for bulding an HTTP Request.
 */

#ifndef AZ_HTTP_REQUEST_BUILDER_H
#define AZ_HTTP_REQUEST_BUILDER_H

#include <az_pair.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_mut_span buffer;
  int16_t max_headers;
  int16_t max_url_size;
  az_const_span method_verb;
  int16_t retry_headers_start;
  int16_t headers_end;
} az_http_request_builder;

/**
 * @brief format buffer as a http request containing headers, url and body.
 */
AZ_NODISCARD az_result az_http_request_builder_init(
    az_http_request_builder * const p_hrb,
    az_mut_span const buffer,
    int16_t const max_url_size,
    az_const_span const method_verb,
    az_const_span const initial_url);

/**
 * @brief set a query parameter. If the query name is not in url yet, it will be added, otherwise
 * modified
 */
AZ_NODISCARD az_result az_http_request_builder_set_query_parameter(
    az_http_request_builder * const p_hrb,
    az_const_span const name,
    az_const_span const value);

/**
 * @brief add a new header for the request.
 */
AZ_NODISCARD az_result az_http_request_builder_append_header(
    az_http_request_builder * const p_hrb,
    az_const_span const name,
    az_const_span const value);

AZ_NODISCARD az_result az_http_request_builder_mark_retry_headers_start(az_http_request_builder * const p_hrb);

AZ_NODISCARD az_result az_http_request_builder_remove_retry_headers(az_http_request_builder * const p_hrb);

AZ_NODISCARD az_result az_http_request_builder_remove_retry_headers(az_http_request_builder * const p_hrb);

#include <_az_cfg_suffix.h>

#endif
