// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_PIPELINE_INTERNAL_H
#define _az_HTTP_PIPELINE_INTERNAL_H

#include <az_http.h>
#include <az_result.h>

#include <_az_cfg_prefix.h>

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

// Required to define az_http_policy for using it to create policy process definition
typedef struct az_http_policy az_http_policy;

typedef AZ_NODISCARD az_result (*az_http_policy_process)(
    az_http_policy * p_policies,
    void * p_options,
    az_http_request * p_request,
    az_http_response * p_response);

struct az_http_policy {
  az_http_policy_process process;
  void * p_options;
};

typedef struct {
  az_http_policy p_policies[9];
} az_http_pipeline;

// Start the pipeline
AZ_NODISCARD az_result az_http_pipeline_process(
    az_http_pipeline * pipeline,
    az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_apiversion(
    az_http_policy * p_policies,
    void * p_data,
    az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_uniquerequestid(
    az_http_policy * p_policies,
    void * p_data,
    az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_retry(
    az_http_policy * p_policies,
    void * p_data,
    az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_authentication(
    az_http_policy * p_policies,
    void * p_data,
    az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_logging(
    az_http_policy * p_policies,
    void * p_data,
    az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_bufferresponse(
    az_http_policy * p_policies,
    void * p_data,
    az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_distributedtracing(
    az_http_policy * p_policies,
    void * p_data,
    az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_transport(
    az_http_policy * p_policies,
    void * p_data,
    az_http_request * p_request,
    az_http_response * p_response);

#include <_az_cfg_suffix.h>
#endif
