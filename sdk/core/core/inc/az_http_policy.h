// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_POLICY_H
#define _az_HTTP_POLICY_H

#include <az_http_request_builder.h>
#include <az_http_response.h>
#include <az_result.h>

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

AZ_NODISCARD az_result az_http_pipeline_policy_uniquerequestid(
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
