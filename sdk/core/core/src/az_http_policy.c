// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_policy.h>

#include <az_contract.h>
#include <az_result.h>
#include <az_str.h>

#include <az_http_pipeline.h>

#include <_az_cfg.h>

inline az_result az_http_pipeline_nextpolicy(
    az_http_policy * const p_policies,
    az_http_request * const p_request,
    az_http_response_data * const out) {

  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(p_request);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  //Transport Policy is the last policy in the pipeline
  //  it returns without calling nextpolicy
  if (&(p_policies[0]) == NULL) {
    return AZ_ERROR_HTTP_PIPELINE_INVALID_POLICY;
  }
  
  az_http_policy_pfnc_process next_policy = p_policies[0].pfnc_process;
  az_http_policy * next_polices = &(p_policies[1]);
  return next_policy(next_polices, p_request, out);
}

az_result az_http_pipeline_policy_uniquerequestid(
    az_http_policy * const p_policies,
    az_http_request * const p_request,
    az_http_response_data * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(p_request);
  AZ_CONTRACT_ARG_NOT_NULL(out);
  // Append the Unique GUID into the headers
  //  x-ms-client-request-id
  return az_http_pipeline_nextpolicy(p_policies, p_request, out);
}

az_result az_http_pipeline_policy_retry(
    az_http_policy * const p_policies,
    az_http_request * const p_request,
    az_http_response_data * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(p_request);
  AZ_CONTRACT_ARG_NOT_NULL(out);
  // Retry logic
  return az_http_pipeline_nextpolicy(p_policies, p_request, out);
}

az_result az_http_pipeline_policy_authentication(
    az_http_policy * const p_policies,
    az_http_request * const p_request,
    az_http_response_data * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(p_request);
  AZ_CONTRACT_ARG_NOT_NULL(out);
  // Authentication logic
  return az_http_pipeline_nextpolicy(p_policies, p_request, out);
}

az_result az_http_pipeline_policy_logging(
    az_http_policy * const p_policies,
    az_http_request * const p_request,
    az_http_response_data * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(p_request);
  AZ_CONTRACT_ARG_NOT_NULL(out);
  // Authentication logic
  return az_http_pipeline_nextpolicy(p_policies, p_request, out);
}

az_result az_http_pipeline_policy_bufferresponse(
    az_http_policy * const p_policies,
    az_http_request * const p_request,
    az_http_response_data * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(p_request);
  AZ_CONTRACT_ARG_NOT_NULL(out);
  // buffer response logic
  //  this might be uStream
  return az_http_pipeline_nextpolicy(p_policies, p_request, out);
}

az_result az_http_pipeline_policy_distributedtracing(
    az_http_policy * const p_policies,
    az_http_request * const p_request,
    az_http_response_data * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(p_request);
  AZ_CONTRACT_ARG_NOT_NULL(out);
  // Distributed tracing logic
  return az_http_pipeline_nextpolicy(p_policies, p_request, out);
}

az_result az_http_pipeline_policy_transport(
    az_http_policy * const p_policies,
    az_http_request * const p_request,
    az_http_response_data * const out) {

  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(p_request);
  AZ_CONTRACT_ARG_NOT_NULL(out);
  // Make the actual request

  return AZ_OK;
}
