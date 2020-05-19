// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_http_transport.h
 *
 * @brief Utilities to be used by HTTP transport policy implementations.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
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
 * @brief Get method of an HTTP request.
 *
 * @remarks This function is expected to be used by transport layer only.
 *
 * @param[in] request The HTTP request from which to get the method.
 * @param[out] out_method Pointer to write the HTTP method to.
 *
 * @retval An #az_result value indicating the result of the operation:
 *         - #AZ_OK if successful
 */
AZ_NODISCARD az_result
az_http_request_get_method(_az_http_request const* request, az_http_method* out_method);

/**
 * @brief Get url from an HTTP request.
 *
 * @remarks This function is expected to be used by transport layer only.
 *
 * @param[in] request The HTTP request from which to get the url.
 * @param[out] out_url Pointer to write the HTTP URL to.
 *
 * @retval An #az_result value indicating the result of the operation:
 *         - #AZ_OK if successful
 */
AZ_NODISCARD az_result az_http_request_get_url(_az_http_request const* request, az_span* out_url);

/**
 * @brief Get body from an HTTP request.
 *
 * @remarks This function is expected to be used by transport layer only.
 *
 * @param[in] request The HTTP request  from which to get the body.
 * @param[out] out_body Pointer to write the HTTP request body to.
 *
 * @retval An #az_result value indicating the result of the operation:
 *         - #AZ_OK if successful
 */
AZ_NODISCARD az_result az_http_request_get_body(_az_http_request const* request, az_span* out_body);

/**
 * @brief This function is expected to be used by transport adapters like curl. Use it to write
 * content from \p source to \p response.
 *
 * @remarks The \p source can be an empty #az_span. If so, nothing will be written.
 *
 * @param[in] response Pointer to an az_http_response.
 * @param[in] source This is an az_span with the content to be written into response.
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if successful
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the \p response buffer is not big enough to contain
 * the \p source content
 */
AZ_NODISCARD az_result az_http_response_append(az_http_response* response, az_span source);

/**
 * @brief Returns the number of headers within the request.
 * Each header is an #az_pair.
 *
 * @param[in] request Pointer to an az_http_request to be used by this function.
 * @return Number of headers in the request.
 */
AZ_NODISCARD int32_t az_http_request_headers_count(_az_http_request const* request);

/**
 * @brief Send an HTTP request through the wire and write the response into \p p_response.
 *
 * @param[in] p_request Points to an az_http_request that contains the settings and data that is
 * used to send the request through the wire.
 * @param[out] p_response Points to an az_http_response where the response from the wire will be
 * written.
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if successful
 *         - #AZ_ERROR_HTTP_RESPONSE_OVERFLOW if there was any issue while trying to write into \p
 * p_response. It might mean that there was not enough space in \p p_response to hold the entire
 * response from the network.
 *         - #AZ_ERROR_HTTP_RESPONSE_COULDNT_RESOLVE_HOST if the url from \p p_request can't be
 * resolved by the http stack and the request was not sent.
 *         - #AZ_ERROR_HTTP_PLATFORM any other issue from the transport layer.
 *
 */
AZ_NODISCARD az_result
az_http_client_send_request(_az_http_request* p_request, az_http_response* p_response);

#include <_az_cfg_suffix.h>

#endif // _az_HTTP_TRANSPORT_H
