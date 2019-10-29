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
#include <az_span_builder.h>
#include <az_str.h>

typedef struct {
  int16_t capacity;
  az_pair * headers_start;
  int16_t size;
  az_pair * retry_headers_start;
  int16_t headers_end;
} az_http_request_headers_info;

typedef struct {
  int16_t capacity;
  int16_t size;
} az_http_request_url_info;

#include <stdint.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span buffer;
  az_const_span method_verb;
  az_span url;
  uint16_t max_url_size;
  uint16_t max_headers;
  uint16_t retry_headers_start;
  uint16_t headers_end;
  az_const_span body;
} az_http_request_builder;

extern az_const_span const AZ_HTTP_METHOD_VERB_GET;
extern az_const_span const AZ_HTTP_METHOD_VERB_HEAD;
extern az_const_span const AZ_HTTP_METHOD_VERB_POST;
extern az_const_span const AZ_HTTP_METHOD_VERB_PUT;
extern az_const_span const AZ_HTTP_METHOD_VERB_DELETE;
extern az_const_span const AZ_HTTP_METHOD_VERB_TRACE;
extern az_const_span const AZ_HTTP_METHOD_VERB_OPTIONS;
extern az_const_span const AZ_HTTP_METHOD_VERB_CONNECT;
extern az_const_span const AZ_HTTP_METHOD_VERB_PATCH;

/**
 * @brief format buffer as a http request containing headers and url.
 */
AZ_NODISCARD az_result az_http_request_builder_init(
    az_http_request_builder * const p_hrb,
    az_span const buffer,
    uint16_t const max_url_size,
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
    az_const_span const key,
    az_const_span const value);

AZ_NODISCARD az_result
az_http_request_builder_mark_retry_headers_start(az_http_request_builder * const p_hrb);

AZ_NODISCARD az_result
az_http_request_builder_remove_retry_headers(az_http_request_builder * const p_hrb);

AZ_NODISCARD az_result az_http_request_builder_get_header(
    az_http_request_builder * const p_hrb,
    uint16_t const index,
    az_pair * const out_result);

/**
 * @brief Adds a body reference for request builder.
 *
 * Returns AZ_ERROR_ARG if body or builder reference are NULL
 *
 */
AZ_NODISCARD AZ_INLINE az_result az_http_request_builder_add_body(
    az_http_request_builder * const p_hrb,
    az_const_span const * const body) {
  AZ_CONTRACT_ARG_NOT_NULL(body);
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  p_hrb->body = *body;
  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif
