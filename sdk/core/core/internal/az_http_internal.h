// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_INTERNAL_H
#define _az_HTTP_INTERNAL_H

#include <az_http.h>
#include <az_result.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD AZ_INLINE _az_http_policy_apiversion_options
_az_http_policy_apiversion_options_default()
{
  return (_az_http_policy_apiversion_options){ 0 };
}

/**
 * @brief Returns the count of headers on the request
 *        Each header is an az_pair
 *
 */
AZ_NODISCARD AZ_INLINE int32_t _az_http_request_headers_count(_az_http_request* request)
{
  // Cast the unsigned sizeof result to ensure the divsion is signed/signed
  return az_span_length(request->_internal.headers) / (int32_t)sizeof(az_pair);
}

// PipelinePolicies
//   Policies are non-allocating caveat the TransportPolicy
//   Transport p_policies can only allocate if the transport layer they call allocates
// Client ->
//  ===HttpPipelinePolicies===
//    UniqueRequestID
//    Retry
//    Authentication
//    Logging
//    Buffer Response
//    Distributed Tracing
//    TransportPolicy
//  ===Transport Layer===
// PipelinePolicies must implement the process function
//

// Start the pipeline
AZ_NODISCARD az_result az_http_pipeline_process(
    _az_http_pipeline* pipeline,
    _az_http_request* p_request,
    az_http_response* p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_apiversion(
    _az_http_policy* p_policies,
    void* p_data,
    _az_http_request* p_request,
    az_http_response* p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_uniquerequestid(
    _az_http_policy* p_policies,
    void* p_data,
    _az_http_request* p_request,
    az_http_response* p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_telemetry(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_retry(
    _az_http_policy* p_policies,
    void* p_data,
    _az_http_request* p_request,
    az_http_response* p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_credential(
    _az_http_policy* p_policies,
    void* p_data,
    _az_http_request* p_request,
    az_http_response* p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_logging(
    _az_http_policy* p_policies,
    void* p_data,
    _az_http_request* p_request,
    az_http_response* p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_transport(
    _az_http_policy* p_policies,
    void* p_data,
    _az_http_request* p_request,
    az_http_response* p_response);

AZ_NODISCARD az_result
az_http_client_send_request(_az_http_request* p_request, az_http_response* p_response);

/**
 * @brief Format buffer as a http request containing URL and header spans.
 *
 * @param p_request HTTP request builder to initialize.
 * @param method HTTP verb: `"GET"`, `"POST"`, etc.
 * @param url Maximum URL length (see @ref az_http_request_builder_set_query_parameter).
 * @param headers_buffer HTTP verb: `"GET"`, `"POST"`, etc.
 * @param body URL.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_BUFFER_OVERFLOW`* `buffer` does not have enough space to fit the `max_url_size`.
 *   - *`AZ_ERROR_ARG`*
 *     - `p_request` is _NULL_.
 *     - `buffer`, `method_verb`, or `initial_url` are invalid spans (see @ref az_span_is_valid).
 *     - `max_url_size` is less than `initial_url.size`.
 */
AZ_NODISCARD az_result az_http_request_init(
    _az_http_request* p_request,
    az_context* context,
    az_http_method method,
    az_span url,
    az_span headers_buffer,
    az_span body);

/**
 * @brief Adds path to url request.
 * For instance, if url in request is `http://example.net?qp=1` and this function is called with
 * path equals to `test`, then request url will be updated to `http://example.net/test?qp=1`.
 *
 *
 * @param p_request http request builder reference
 * @param path span to a path to be appended into url
 * @return AZ_NODISCARD az_http_request_builder_append_path
 */
AZ_NODISCARD az_result az_http_request_append_path(_az_http_request* p_request, az_span path);

/**
 * @brief Set query parameter.
 *
 * @param p_request HTTP request builder that holds the URL to set the query parameter to.
 * @param name URL parameter name.
 * @param value URL parameter value.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_BUFFER_OVERFLOW`* the `URL` would grow past the `max_url_size`, should the
 * parameter gets set.
 *   - *`AZ_ERROR_ARG`*
 *     - `p_request` is _NULL_.
 *     - `name` or `value` are invalid spans (see @ref az_span_is_valid).
 *     - `name` or `value` are empty.
 *     - `name`'s or `value`'s buffer overlap resulting `url`'s buffer.
 */
AZ_NODISCARD az_result
az_http_request_set_query_parameter(_az_http_request* p_request, az_span name, az_span value);

/**
 * @brief Add a new HTTP header for the request.
 *
 * @param p_request HTTP request builder that holds the URL to set the query parameter to.
 * @param key Header name (e.g. `"Content-Type"`).
 * @param value Header value (e.g. `"application/x-www-form-urlencoded"`).
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_BUFFER_OVERFLOW`* there isn't enough space in the `p_request->buffer` to add a
 * header.
 *   - *`AZ_ERROR_ARG`*
 *     - `p_request` is _NULL_.
 *     - `key` or `value` are invalid spans (see @ref az_span_is_valid).
 *     - `key` or `value` are empty.
 *     - `name`'s or `value`'s buffer overlap resulting `url`'s buffer.
 */
AZ_NODISCARD az_result
az_http_request_append_header(_az_http_request* p_request, az_span key, az_span value);

#include <_az_cfg_suffix.h>

#endif // _az_HTTP_INTERNAL_H
