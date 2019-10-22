// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_REQUEST_BUILDER_H
#define AZ_HTTP_REQUEST_BUILDER_H

#include <_az_cfg_prefix.h>

#include <az_pair.h>
#include <az_span.h>
#include <az_write_span_iter.h>

#define AZ_HTTP_REQUEST_DEFAULT_URL_SIZE 1024
#define AZ_HTTP_REQUEST_DEFAULT_HEADER_SIZE sizeof(az_pair)
#define AZ_HTTP_REQUEST_DEFAULT_MAX_HEADERS 50
#define AZ_HTTP_REQUEST_DEFAULT_MAX_BODY_SIZE (AZ_HTTP_REQUEST_DEFAULT_URL_SIZE * 1)

typedef struct {
  az_span * buffer;
  int16_t max_headers;
  int16_t max_url_size;
  az_const_span method_verb;
  int16_t retry_headers_start;
  int16_t headers_end;
} az_http_request_builder;

typedef struct {
  int16_t const max_url_size;
  int16_t const max_headers;
  int16_t const min_buffer_size;
} az_http_request_defaults;

/**
 * @brief format buffer as a http request containing headers, url and body. Url max size is
 * initially set to a default size. use `az_http_request_builder_set_max_url_size` after init to
 * change this value if required
 *
 * @param out
 * @param buffer
 * @param method_verb
 * @return az_result
 */
az_result az_http_request_builder_init(
    az_http_request_builder * const out,
    az_span * const buffer,
    az_const_span const method_verb);

az_result az_http_request_builder_set_initial_url(
    az_http_request_builder * const p_builder,
    az_const_span const url);
/*
az_result az_http_request_builder_set_query_parameter(
    az_http_request_builder * const p_builder,
    az_const_span const name,
    az_const_span const value);

az_result az_http_request_builder_append_header(
    az_http_request_builder * const p_builder,
    az_const_span const name,
    az_const_span const value);

az_result az_http_request_builder_mark_retry_headers_start(
    az_http_request_builder * const p_builder);

az_result az_http_request_builder_remove_retry_headers(az_http_request_builder * const p_builder);
*/

AZ_INLINE az_result az_http_request_builder_set_max_url_size(
    az_http_request_builder * const p_builder,
    int16_t const size) {
  p_builder->max_url_size = size;
  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif
