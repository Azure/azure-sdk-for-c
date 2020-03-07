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

#include <_az_cfg_prefix.h>

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
 * @brief Get the HTTP header by index.
 *
 * @param request HTTP request to get HTTP header from.
 * @param index Index of the HTTP header to get.
 * @param out_result Pointer to write the result to.
 *
 * @retval AZ_OK Success.
 * @retval AZ_ERROR_ARG \a index is out of range.
 */
AZ_NODISCARD az_result
az_http_request_get_header(_az_http_request const* request, int32_t index, az_pair* out_result);

/**
 * @brief Get parts of an HTTP request.
 *
 * @param request HTTP request to get parts from.
 * @param out_method Pointer to write HTTP method to.
 * @param out_method Pointer to write URL to.
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
