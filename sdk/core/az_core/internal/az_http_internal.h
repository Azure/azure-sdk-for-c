// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_INTERNAL_H
#define _az_HTTP_INTERNAL_H

#include <az_context.h>
#include <az_http.h>
#include <az_http_transport.h>
#include <az_result.h>

#include <_az_cfg_prefix.h>

/**
 * @brief Internal definition of an HTTP pipeline.
 *
 * Defines the number of policies inside a pipeline.
 *
 * Users @b should @b not access _internal field.
 *
 */
typedef struct
{
  struct
  {
    _az_http_policy policies[10];
  } _internal;
} _az_http_pipeline;

typedef enum
{
  _az_http_policy_apiversion_option_location_header,
  _az_http_policy_apiversion_option_location_queryparameter
} _az_http_policy_apiversion_option_location;

/**
 * @brief Defines the options structure used by the api version policy
 *
 * Users @b should @b not access _internal field.
 *
 */
typedef struct
{
  // Services pass API versions in the header or in query parameters
  struct
  {
    _az_http_policy_apiversion_option_location option_location;
    az_span name;
    az_span version;
  } _internal;
} _az_http_policy_apiversion_options;

/**
 * @brief options for the telemetry policy
 * os = string representation of currently executing Operating System
 *
 */
typedef struct
{
  az_span os;
} _az_http_policy_telemetry_options;

/**
 * @brief Initialize _az_http_policy_telemetry_options with default values
 *
 */
AZ_NODISCARD AZ_INLINE _az_http_policy_telemetry_options _az_http_policy_telemetry_options_default()
{
  return (_az_http_policy_telemetry_options){ .os = AZ_SPAN_FROM_STR("Unknown OS") };
}

AZ_NODISCARD AZ_INLINE _az_http_policy_apiversion_options
_az_http_policy_apiversion_options_default()
{
  return (_az_http_policy_apiversion_options){
    ._internal = { .option_location = _az_http_policy_apiversion_option_location_header,
                   .name = AZ_SPAN_NULL,
                   .version = AZ_SPAN_NULL }
  };
}

/**
 * @brief Initialize az_http_policy_retry_options with default values
 *
 */
AZ_NODISCARD az_http_policy_retry_options _az_http_policy_retry_options_default();

// PipelinePolicies
//   Policies are non-allocating caveat the TransportPolicy
//   Transport policies can only allocate if the transport layer they call allocates
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
    _az_http_pipeline* ref_pipeline,
    _az_http_request* ref_request,
    az_http_response* ref_response);

AZ_NODISCARD az_result az_http_pipeline_policy_apiversion(
    _az_http_policy* ref_policies,
    void* ref_options,
    _az_http_request* ref_request,
    az_http_response* ref_response);

AZ_NODISCARD az_result az_http_pipeline_policy_telemetry(
    _az_http_policy* ref_policies,
    void* ref_options,
    _az_http_request* ref_request,
    az_http_response* ref_response);

AZ_NODISCARD az_result az_http_pipeline_policy_retry(
    _az_http_policy* ref_policies,
    void* ref_options,
    _az_http_request* ref_request,
    az_http_response* ref_response);

AZ_NODISCARD az_result az_http_pipeline_policy_credential(
    _az_http_policy* ref_policies,
    void* ref_options,
    _az_http_request* ref_request,
    az_http_response* ref_response);

#ifndef AZ_NO_LOGGING
AZ_NODISCARD az_result az_http_pipeline_policy_logging(
    _az_http_policy* ref_policies,
    void* ref_options,
    _az_http_request* ref_request,
    az_http_response* ref_response);
#endif // AZ_NO_LOGGING

AZ_NODISCARD az_result az_http_pipeline_policy_transport(
    _az_http_policy* ref_policies,
    void* ref_options,
    _az_http_request* ref_request,
    az_http_response* ref_response);

AZ_NODISCARD AZ_INLINE az_result _az_http_pipeline_nextpolicy(
    _az_http_policy* ref_policies,
    _az_http_request* ref_request,
    az_http_response* ref_response)
{
  // Transport Policy is the last policy in the pipeline
  //  it returns without calling nextpolicy
  if (ref_policies[0]._internal.process == NULL)
  {
    return AZ_ERROR_HTTP_PIPELINE_INVALID_POLICY;
  }

  return ref_policies[0]._internal.process(
      &(ref_policies[1]), ref_policies[0]._internal.options, ref_request, ref_response);
}

/**
 * @brief Format buffer as a http request containing URL and header spans.
 *
 * @param out_request HTTP request builder to initialize.
 * @param method HTTP verb: `"GET"`, `"POST"`, etc.
 * @param url Maximum URL length (see @ref az_http_request_set_query_parameter).
 * @param headers_buffer HTTP verb: `"GET"`, `"POST"`, etc.
 * @param body URL.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_INSUFFICIENT_SPAN_SIZE`* `buffer` does not have enough space to fit the
 * `max_url_size`.
 *   - *`AZ_ERROR_ARG`*
 *     - `ref_request` is _NULL_.
 *     - `buffer`, `method_verb`, or `initial_url` are invalid spans (see @ref _az_span_is_valid).
 *     - `max_url_size` is less than `initial_url.size`.
 */
AZ_NODISCARD az_result az_http_request_init(
    _az_http_request* out_request,
    az_context* context,
    az_http_method method,
    az_span url,
    int32_t url_length,
    az_span headers_buffer,
    az_span body);

/**
 * @brief Adds path to url request.
 * For instance, if url in request is `http://example.net?qp=1` and this function is called with
 * path equals to `test`, then request url will be updated to `http://example.net/test?qp=1`.
 *
 *
 * @param ref_request http request builder reference
 * @param path span to a path to be appended into url
 * @return AZ_NODISCARD az_http_request_append_path
 */
AZ_NODISCARD az_result az_http_request_append_path(_az_http_request* ref_request, az_span path);

/**
 * @brief Set query parameter.
 *
 * @param ref_request HTTP request builder that holds the URL to set the query parameter to.
 * @param name URL parameter name.
 * @param value URL parameter value.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_INSUFFICIENT_SPAN_SIZE`* the `URL` would grow past the `max_url_size`, should
 * the parameter get set.
 *   - *`AZ_ERROR_ARG`*
 *     - `p_request` is _NULL_.
 *     - `name` or `value` are invalid spans (see @ref _az_span_is_valid).
 *     - `name` or `value` are empty.
 *     - `name`'s or `value`'s buffer overlap resulting `url`'s buffer.
 */
AZ_NODISCARD az_result
az_http_request_set_query_parameter(_az_http_request* ref_request, az_span name, az_span value);

/**
 * @brief Add a new HTTP header for the request.
 *
 * @param ref_request HTTP request builder that holds the URL to set the query parameter to.
 * @param key Header name (e.g. `"Content-Type"`).
 * @param value Header value (e.g. `"application/x-www-form-urlencoded"`).
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_INSUFFICIENT_SPAN_SIZE`* there isn't enough space in the `p_request->buffer`
 * to add a header.
 *   - *`AZ_ERROR_ARG`*
 *     - `p_request` is _NULL_.
 *     - `key` or `value` are invalid spans (see @ref _az_span_is_valid).
 *     - `key` or `value` are empty.
 *     - `name`'s or `value`'s buffer overlap resulting `url`'s buffer.
 */
AZ_NODISCARD az_result
az_http_request_append_header(_az_http_request* ref_request, az_span key, az_span value);

#include <_az_cfg_suffix.h>

#endif // _az_HTTP_INTERNAL_H
