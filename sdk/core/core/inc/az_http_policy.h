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

typedef struct {
  az_span data;
} az_http_policy_data;

// PipelinePolicies must implement the process function
//
typedef AZ_NODISCARD az_result (*az_http_policy_pfnc_process)(
    az_http_policy * policies,
    az_http_request_builder * hrb,
    az_mut_span const * const response);

struct az_http_policy {
  az_http_policy_pfnc_process pfnc_process;
  az_http_policy_data data;
};

AZ_NODISCARD az_result az_http_pipeline_policy_uniquerequestid(
    az_http_policy * const policies,
    az_http_request_builder * const hrb,
    az_mut_span const * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_retry(
    az_http_policy * const policies,
    az_http_request_builder * const hrb,
    az_mut_span const * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_authentication(
    az_http_policy * const policies,
    az_http_request_builder * const hrb,
    az_mut_span const * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_logging(
    az_http_policy * const policies,
    az_http_request_builder * const hrb,
    az_mut_span const * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_bufferresponse(
    az_http_policy * const policies,
    az_http_request_builder * const hrb,
    az_mut_span const * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_distributedtracing(
    az_http_policy * const policies,
    az_http_request_builder * const hrb,
    az_mut_span const * const response);

AZ_NODISCARD az_result az_http_pipeline_policy_transport(
    az_http_policy * const policies,
    az_http_request_builder * const hrb,
    az_mut_span const * const response);

#include <_az_cfg_suffix.h>

#endif
