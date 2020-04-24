// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_http_transport.h
 *
 * @brief Utilities to be used by HTTP transport policy implementations.
 *
 * NOTE: You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
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
    int32_t url_length;
    int32_t query_start;
    _az_http_request_headers headers; // Contains az_pairs
    int32_t headers_length;
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
 * @brief Get parts of an HTTP request. `NULL` in accepted to ignore getting any parts, for example,
 * call this function like below to get only the http method and ignore getting url and body.
 *   `az_http_request_get_parts(request, &method, NULL, NULL)`
 *
 * This function is expected to be used by transport layer only.
 *
 * @param request HTTP request to get parts from.
 * @param[out] out_method __[nullable]__ Pointer to write HTTP method to. Use `NULL` to ignore
 * getting this value.
 * @param[out] out_url __[nullable]__ Pointer to write URL to. Use `NULL` to ignore getting this
 * value.
 * @param[out] out_bodyp __[nullable]__ Pointer to write HTTP request body to. Use `NULL` to ignore
 * getting this value.
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
