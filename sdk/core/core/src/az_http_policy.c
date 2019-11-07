// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_policy.h>

#include <az_contract.h>
#include <az_result.h>
#include <az_str.h>

#include <az_http_pipeline.h>

#include <_az_cfg.h>

AZ_INLINE az_result az_http_pipeline_nextpolicy(
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_http_response_data * const out) {

  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  //Transport Policy is the last policy in the pipeline
  //  it returns without calling nextpolicy
  if (&(p_policies[0]) == NULL || p_policies[0].pfnc_process == NULL) {
    return AZ_ERROR_HTTP_PIPELINE_INVALID_POLICY;
  }

  return p_policies[0].pfnc_process(&(p_policies[1]), hrb, out);
}

static az_span const AZ_MS_CLIENT_REQUESTID = AZ_CONST_STR("x-ms-client-request-id");

az_result az_http_pipeline_policy_uniquerequestid(
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_http_response_data * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  //TODO - add a UUID create implementation
  az_span const uniqueid = AZ_CONST_STR("123e4567-e89b-12d3-a456-426655440000");


  // Append the Unique GUID into the headers
  //  x-ms-client-request-id
  az_result add_header_result = az_http_request_builder_append_header(
      hrb, AZ_MS_CLIENT_REQUESTID, uniqueid);
  if (az_failed(add_header_result)) {
    return add_header_result;
  }

  return az_http_pipeline_nextpolicy(p_policies, hrb, out);
}

az_result az_http_pipeline_policy_retry(
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_http_response_data * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(out);
  // Retry logic
  return az_http_pipeline_nextpolicy(p_policies, hrb, out);
}

az_result az_http_pipeline_policy_authentication(
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_http_response_data * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(out);
  // Authentication logic
  return az_http_pipeline_nextpolicy(p_policies, hrb, out);
}

az_result az_http_pipeline_policy_logging(
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_http_response_data * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(out);
  // Authentication logic
  return az_http_pipeline_nextpolicy(p_policies, hrb, out);
}

az_result az_http_pipeline_policy_bufferresponse(
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_http_response_data * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(out);
  // buffer response logic
  //  this might be uStream
  return az_http_pipeline_nextpolicy(p_policies, hrb, out);
}

az_result az_http_pipeline_policy_distributedtracing(
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_http_response_data * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(out);
  // Distributed tracing logic
  return az_http_pipeline_nextpolicy(p_policies, hrb, out);
}

az_result az_http_pipeline_policy_transport(
    az_http_policy * const p_policies,
    az_http_request_builder * const hrb,
    az_http_response_data * const out) {

  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(out);
  // Make the actual request

  // Transport policy is the last policy
  //  If a policy exists after the transport policy
  if(p_policies[0].pfnc_process != NULL) {
    return AZ_ERROR_HTTP_PIPELINE_INVALID_POLICY;
  }

  return AZ_OK;
}
