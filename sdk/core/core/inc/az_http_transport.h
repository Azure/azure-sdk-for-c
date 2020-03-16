// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_http_transport.h
 *
 * @brief Utilities to be used by HTTP transport policy implementations.
 */

#ifndef _az_HTTP_TRANSPORT_H
#define _az_HTTP_TRANSPORT_H

#include <az_http.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

/**
 * @brief az_http_method is a type representing an HTTP method (POST, PUT, GET, DELETE, etc.).
 */
typedef az_span az_http_method;

/// HTTP GET method name.
AZ_INLINE az_http_method az_http_method_get() { return AZ_SPAN_FROM_STR("GET"); }

/// HTTP HEAD method name.
AZ_INLINE az_http_method az_http_method_head() { return AZ_SPAN_FROM_STR("HEAD"); }

/// HTTP POST method name.
AZ_INLINE az_http_method az_http_method_post() { return AZ_SPAN_FROM_STR("POST"); }

/// HTTP PUT method name.
AZ_INLINE az_http_method az_http_method_put() { return AZ_SPAN_FROM_STR("PUT"); }

/// HTTP DELETE method name.
AZ_INLINE az_http_method az_http_method_delete() { return AZ_SPAN_FROM_STR("DELETE"); }

/// HTTP PATCH method name.
AZ_INLINE az_http_method az_http_method_patch() { return AZ_SPAN_FROM_STR("PATCH"); }

/**
 * @brief _az_http_request_headers is a type representing a buffer of az_pair instances for HTTP
 * request headers.
 */
typedef az_span _az_http_request_headers;

/**
 * @brief _az_http_request is an internal structure used to perform an HTTP request.
 * It contains an HTTP method, url, headers and body. It also contains
 * another utility variables. User should never access field _internal directly
 */
typedef struct
{
  struct
  {
    az_context* context;
    az_http_method method;
    az_span url;
    int32_t query_start;
    _az_http_request_headers headers; // Contains az_pairs
    int32_t max_headers;
    int32_t retry_headers_start_byte_offset;
    az_span body;
  } _internal;
} _az_http_request;

/**
 * @brief Get the HTTP header by index.
 *
 * @param request HTTP request to get HTTP header from.
 * @param index Index of the HTTP header to get.
 * @param out_header Pointer to write the result to.
 *
 * @retval AZ_OK Success.
 * @retval AZ_ERROR_ARG \a index is out of range.
 */
AZ_NODISCARD az_result
az_http_request_get_header(_az_http_request const* request, int32_t index, az_pair* out_header);

/**
 * @brief Get parts of an HTTP request.
 *
 * @param request HTTP request to get parts from.
 * @param out_method Pointer to write HTTP method to.
 * @param out_url Pointer to write URL to.
 * @param out_body Pointer to write HTTP request body to.
 *
 * @retval AZ_OK Success.
 */
AZ_NODISCARD az_result az_http_request_get_parts(
    _az_http_request const* request,
    az_http_method* out_method,
    az_span* out_url,
    az_span* out_body);

#include <_az_cfg_suffix.h>

#endif // _az_HTTP_TRANSPORT_H
