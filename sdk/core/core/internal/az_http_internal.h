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

// Start the pipeline
AZ_NODISCARD az_result az_http_pipeline_process(
    _az_http_pipeline * pipeline,
    _az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_apiversion(
    _az_http_policy * p_policies,
    void * p_data,
    _az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_uniquerequestid(
    _az_http_policy * p_policies,
    void * p_data,
    _az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_telemetry(
    _az_http_policy * p_policies,
    void * p_options,
    _az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_retry(
    _az_http_policy * p_policies,
    void * p_data,
    _az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_credential(
    _az_http_policy * p_policies,
    void * p_data,
    _az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_logging(
    _az_http_policy * p_policies,
    void * p_data,
    _az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_bufferresponse(
    _az_http_policy * p_policies,
    void * p_data,
    _az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_distributedtracing(
    _az_http_policy * p_policies,
    void * p_data,
    _az_http_request * p_request,
    az_http_response * p_response);

AZ_NODISCARD az_result az_http_pipeline_policy_transport(
    _az_http_policy * p_policies,
    void * p_data,
    _az_http_request * p_request,
    az_http_response * p_response);

#include <_az_cfg_suffix.h>
#endif
