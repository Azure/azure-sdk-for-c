// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_http_request_builder.h
 * @author your name (you@domain.com)
 * @brief Interface declaration for bulding an HTTP Request.
 * @date 2019-10-23
 *
 */

#ifndef AZ_HTTP_REQUEST_BUILDER_H
#define AZ_HTTP_REQUEST_BUILDER_H

#include <_az_cfg_prefix.h>

#include <az_pair.h>
#include <az_span.h>

typedef struct {
  az_span buffer;
  int16_t max_headers;
  int16_t max_url_size;
  az_const_span method_verb;
  int16_t retry_headers_start;
  int16_t headers_end;
} az_http_request_builder;

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
    az_http_request_builder * const p_hrb,
    az_span const buffer,
    int16_t const max_url_size,
    az_const_span const method_verb,
    az_const_span const initial_url);

/**
 * @brief set a query parameter. If the query name is not in url yet, it will be added, otherwise
 * modified
 *
 * @param p_hrb
 * @param name
 * @param value
 * @return az_result
 */
az_result az_http_request_builder_set_query_parameter(
    az_http_request_builder * const p_hrb,
    az_const_span const name,
    az_const_span const value);

/**
 * @brief add a new header for the request.
 *
 * @param p_hrb
 * @param name
 * @param value
 * @return az_result
 */
az_result az_http_request_builder_append_header(
    az_http_request_builder * const p_hrb,
    az_const_span const name,
    az_const_span const value);

/**
 * @brief
 *
 * @param p_hrb
 * @return az_result
 */
az_result az_http_request_builder_mark_retry_headers_start(az_http_request_builder * const p_hrb);

/**
 * @brief
 *
 * @param p_hrb
 * @return az_result
 */
az_result az_http_request_builder_remove_retry_headers(az_http_request_builder * const p_hrb);

/**
 * @brief
 *
 * @param p_hrb
 * @return az_result
 */
az_result az_http_request_builder_remove_retry_headers(az_http_request_builder * const p_hrb);

#include <_az_cfg_suffix.h>

#endif
