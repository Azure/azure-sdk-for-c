// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
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

#include <azure/core/az_http.h>
#include <azure/core/az_span.h>

#include <azure/core/_az_cfg_prefix.h>

/// @brief A type representing an HTTP method (`POST`, `PUT`, `GET`, `DELETE`, etc.).
typedef az_span az_http_method;

/// @brief HTTP GET method name.
AZ_INLINE az_http_method az_http_method_get() { return AZ_SPAN_FROM_STR("GET"); }

/// @brief HTTP HEAD method name.
AZ_INLINE az_http_method az_http_method_head() { return AZ_SPAN_FROM_STR("HEAD"); }

/// @brief HTTP POST method name.
AZ_INLINE az_http_method az_http_method_post() { return AZ_SPAN_FROM_STR("POST"); }

/// @brief HTTP PUT method name.
AZ_INLINE az_http_method az_http_method_put() { return AZ_SPAN_FROM_STR("PUT"); }

/// @brief HTTP DELETE method name.
AZ_INLINE az_http_method az_http_method_delete() { return AZ_SPAN_FROM_STR("DELETE"); }

/// @brief HTTP PATCH method name.
AZ_INLINE az_http_method az_http_method_patch() { return AZ_SPAN_FROM_STR("PATCH"); }

// A type representing a buffer of az_pair instances for HTTP request headers.
typedef az_span _az_http_request_headers;

/**
 * @brief Structure used to represent an HTTP request.
 * It contains an HTTP method, URL, headers and body. It also contains
 * another utility variables. User should never access `_internal` field directly.
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
} az_http_request;

// Declaring #_az_http_policy for using it to create policy process callback
// #_az_http_policy_process_fn definition. Definition is added below after it.
typedef struct _az_http_policy _az_http_policy;

// @brief Defines the callback signature of a policy process which should receive an
// #_az_http_policy, options reference (as `void*`), an #az_http_request and #az_http_response.
//
// `void*` is used as polymorphic solution for any policy. Each policy implementation would know the
// specic pointer type to cast options to.
typedef AZ_NODISCARD az_result (*_az_http_policy_process_fn)(
    _az_http_policy* ref_policies,
    void* ref_options,
    az_http_request* ref_request,
    az_http_response* ref_response);

// @brief Definition for an HTTP policy.
//
// An HTTP pipeline inside SDK clients is an array of http policies.
//
// Users @b should @b not access `_internal` field where process callback and options are defined.
struct _az_http_policy
{
  struct
  {
    _az_http_policy_process_fn process;
    void* options;
  } _internal;
};

/**
 * @brief Gets the HTTP header by index.
 *
 * @param[in] request HTTP request to get HTTP header from.
 * @param[in] index Index of the HTTP header to get.
 * @param[out] out_header Pointer to write the result to.
 *
 * @retval #AZ_OK Success.
 * @retval #AZ_ERROR_ARG \a index is out of range.
 */
AZ_NODISCARD az_result
az_http_request_get_header(az_http_request const* request, int32_t index, az_pair* out_header);

/**
 * @brief Get method of an HTTP request.
 *
 * @remarks This function is expected to be used by transport layer only.
 *
 * @param[in] request The HTTP request from which to get the method.
 * @param[out] out_method Pointer to write the HTTP method to.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval other Failure.
 */
AZ_NODISCARD az_result
az_http_request_get_method(az_http_request const* request, az_http_method* out_method);

/**
 * @brief Get URL from an HTTP request.
 *
 * @remarks This function is expected to be used by transport layer only.
 *
 * @param[in] request The HTTP request from which to get the URL.
 * @param[out] out_url Pointer to write the HTTP URL to.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval other Failure.
 */
AZ_NODISCARD az_result az_http_request_get_url(az_http_request const* request, az_span* out_url);

/**
 * @brief Get body from an HTTP request.
 *
 * @remarks This function is expected to be used by transport layer only.
 *
 * @param[in] request The HTTP request from which to get the body.
 * @param[out] out_body Pointer to write the HTTP request body to.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval other Failure.
 */
AZ_NODISCARD az_result az_http_request_get_body(az_http_request const* request, az_span* out_body);

/**
 * @brief This function is expected to be used by transport adapters like curl. Use it to write
 * content from \p source to \p ref_response.
 *
 * @remarks The \p source can be an empty #az_span. If so, nothing will be written.
 *
 * @param[in,out] ref_response Pointer to an #az_http_response.
 * @param[in] source This is an #az_span with the content to be written into \p ref_response.
 *
 * @retval #AZ_OK Success.
 * @retval #AZ_ERROR_INSUFFICIENT_SPAN_SIZE The \p response buffer is not big enough to contain the
 * \p source content.
 */
AZ_NODISCARD az_result az_http_response_append(az_http_response* ref_response, az_span source);

/**
 * @brief Returns the number of headers within the request.
 * Each header is an #az_pair.
 *
 * @param[in] request Pointer to an #az_http_request to be used by this function.
 *
 * @return Number of headers in the /p request.
 */
AZ_NODISCARD int32_t az_http_request_headers_count(az_http_request const* request);

/**
 * @brief Sends an HTTP request through the wire and write the response into \p ref_response.
 *
 * @param[in] request Points to an #az_http_request that contains the settings and data that is
 * used to send the request through the wire.
 * @param[in,out] ref_response Points to an #az_http_response where the response from the wire will
 * be written.
 *
 * @retval #AZ_OK Success.
 * @retval #AZ_ERROR_HTTP_RESPONSE_OVERFLOW There was any issue while trying to write into \p
 * ref_response. It might mean that there was not enough space in \p ref_response to hold the entire
 * response from the network.
 * @retval #AZ_ERROR_HTTP_RESPONSE_COULDNT_RESOLVE_HOST The URL from \p ref_request can't be
 * resolved by the HTTP stack and the request was not sent.
 * @retval #AZ_ERROR_HTTP_ADAPTER Any other issue from the transport adapter layer.
 */
AZ_NODISCARD az_result
az_http_client_send_request(az_http_request const* request, az_http_response* ref_response);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_HTTP_TRANSPORT_H
