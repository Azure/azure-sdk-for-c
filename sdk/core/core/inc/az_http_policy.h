// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_POLICY_H
#define _az_HTTP_POLICY_H

#include <az_http_request_builder.h>
#include <az_http_response.h>
#include <az_result.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

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

typedef struct az_http_policy az_http_policy;

/**
 * @brief options for the apiversion policy
 * add_as_header = Services pass API versions in the header or in query parameters
 * name = header or query parameter name
 * version = string representation of the targeted service version
 *
 */
typedef struct {
  // Services pass API versions in the header or in query parameters
  //   true: api version is passed via headers
  //   false: api version is passed via query parameters
  bool add_as_header;
  az_span name;
  az_span version;
} _az_http_policy_apiversion_options;

/**
 * @brief options for the telemetry policy
 * os = string representation of currently executing Operating System
 *
 */
typedef struct {
  az_span os;
} _az_http_policy_telemetry_options;

/**
 * @brief options for retry policy
 * max_retry = maximun number of retry intents before returning error
 * delay_in_ms = waiting time before retrying in miliseconds
 *
 */
typedef struct {
  uint16_t max_retry;
  uint16_t delay_in_ms;
} az_http_policy_retry_options;

// PipelinePolicies must implement the process function
//
typedef AZ_NODISCARD az_result (*az_http_policy_pfnc_process)(
    az_http_policy * policies,
    void * const data,
    az_http_request_builder * hrb,
    az_http_response * const response);

struct az_http_policy {
  az_http_policy_pfnc_process pfnc_process;
  void * data;
};

AZ_NODISCARD az_result az_http_pipeline_policy_apiversion(
    az_http_policy * const policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_uniquerequestid(
    az_http_policy * const policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response);

AZ_NODISCARD az_result
_az_http_policy_telemetry_options_init(_az_http_policy_telemetry_options * const self);

AZ_NODISCARD az_result az_http_pipeline_policy_telemetry(
    az_http_policy * const policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_retry(
    az_http_policy * const policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_authentication(
    az_http_policy * const policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_logging(
    az_http_policy * const policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_bufferresponse(
    az_http_policy * const policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_distributedtracing(
    az_http_policy * const policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_transport(
    az_http_policy * const policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_http_response * const response);

#include <_az_cfg_suffix.h>

#endif
