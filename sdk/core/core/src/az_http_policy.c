// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_client.h>
#include <az_http_pipeline.h>
#include <az_http_policy.h>

#include <_az_cfg.h>

AZ_NODISCARD AZ_INLINE az_result
az_http_pipeline_nextpolicy(az_http_policy * const p_policies, az_http_policy_arg * const arg) {

  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(arg);

  // Transport Policy is the last policy in the pipeline
  //  it returns without calling nextpolicy
  if (p_policies->pfnc_process == NULL) {
    return AZ_ERROR_HTTP_PIPELINE_INVALID_POLICY;
  }

  return p_policies->pfnc_process(p_policies + 1, arg);
}

static az_span const AZ_MS_CLIENT_REQUESTID = AZ_CONST_STR("x-ms-client-request-id");

AZ_NODISCARD az_result az_http_pipeline_policy_uniquerequestid(
    az_http_policy * const p_policies,
    az_http_policy_arg * const arg) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(arg);

  // TODO - add a UUID create implementation
  az_span const uniqueid = AZ_CONST_STR("123e4567-e89b-12d3-a456-426655440000");

  // Append the Unique GUID into the headers
  //  x-ms-client-request-id
  AZ_RETURN_IF_FAILED(
      az_http_request_builder_append_header(&arg->builder, AZ_MS_CLIENT_REQUESTID, uniqueid));

  return az_http_pipeline_nextpolicy(p_policies, arg);
}

AZ_NODISCARD az_result
az_http_pipeline_policy_retry(az_http_policy * const p_policies, az_http_policy_arg * const arg) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(arg);
  // Retry logic
  return az_http_pipeline_nextpolicy(p_policies, arg);
}

AZ_NODISCARD az_result az_http_pipeline_policy_authentication(
    az_http_policy * const p_policies,
    az_http_policy_arg * const arg) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(arg);
  // Authentication logic
  return az_http_pipeline_nextpolicy(p_policies, arg);
}

AZ_NODISCARD az_result
az_http_pipeline_policy_logging(az_http_policy * const p_policies, az_http_policy_arg * const arg) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(arg);
  // Authentication logic
  return az_http_pipeline_nextpolicy(p_policies, arg);
}

AZ_NODISCARD az_result az_http_pipeline_policy_bufferresponse(
    az_http_policy * const p_policies,
    az_http_policy_arg * const arg) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(arg);
  // buffer response logic
  //  this might be uStream
  return az_http_pipeline_nextpolicy(p_policies, arg);
}

AZ_NODISCARD az_result az_http_pipeline_policy_distributedtracing(
    az_http_policy * const p_policies,
    az_http_policy_arg * const arg) {
  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(arg);
  // Distributed tracing logic
  return az_http_pipeline_nextpolicy(p_policies, arg);
}

AZ_NODISCARD az_result az_http_pipeline_policy_transport(
    az_http_policy * const p_policies,
    az_http_policy_arg * const arg) {

  AZ_CONTRACT_ARG_NOT_NULL(p_policies);
  AZ_CONTRACT_ARG_NOT_NULL(arg);

  // Transport policy is the last policy
  //  If a policy exists after the transport policy
  if (p_policies[0].pfnc_process != NULL) {
    return AZ_ERROR_HTTP_PIPELINE_INVALID_POLICY;
  }

  return az_http_client_send_request(&arg->builder, &arg->response);
}
