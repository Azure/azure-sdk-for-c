// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_POLICY_H
#define AZ_HTTP_POLICY_H

#include <az_http_request_builder.h>
#include <az_mut_span.h>
#include <az_result.h>
#include <az_span.h>

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
 * @brief options for retry policy
 * max_retry = maximun number of retry intents before returning error
 * delay_in_ms = waiting time before retrying in miliseconds
 *
 */
typedef struct {
  uint16_t max_retry;
  uint16_t delay_in_ms;
} az_keyvault_keys_client_options_retry;

// PipelinePolicies must implement the process function
//
typedef AZ_NODISCARD az_result (*az_http_policy_pfnc_process)(
    az_http_policy * policies,
    void * const data,
    az_http_request_builder * hrb,
    az_mut_span const * const response);

struct az_http_policy {
  az_http_policy_pfnc_process pfnc_process;
  void * data;
};

AZ_NODISCARD az_result az_http_pipeline_policy_uniquerequestid(
    az_http_policy * const policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_mut_span const * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_retry(
    az_http_policy * const policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_mut_span const * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_authentication(
    az_http_policy * const policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_mut_span const * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_logging(
    az_http_policy * const policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_mut_span const * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_bufferresponse(
    az_http_policy * const policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_mut_span const * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_distributedtracing(
    az_http_policy * const policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_mut_span const * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_transport(
    az_http_policy * const policies,
    void * const data,
    az_http_request_builder * const hrb,
    az_mut_span const * const response);

#include <_az_cfg_suffix.h>

#endif
