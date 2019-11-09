// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_pipeline.h>
#include <az_http_policy.h>

#include <_az_cfg.h>

az_http_policy pipeline[] = {
  { .pfnc_process = az_http_pipeline_policy_uniquerequestid, .data = { 0 } },
  { .pfnc_process = az_http_pipeline_policy_retry, .data = { 0 } },
  { .pfnc_process = az_http_pipeline_policy_authentication, .data = { 0 } },
  { .pfnc_process = az_http_pipeline_policy_logging, .data = { 0 } },
  { .pfnc_process = az_http_pipeline_policy_bufferresponse, .data = { 0 } },
  { .pfnc_process = az_http_pipeline_policy_distributedtracing, .data = { 0 } },
  { .pfnc_process = az_http_pipeline_policy_transport, .data = { 0 } },
  { .pfnc_process = NULL, .data = { 0 } },
};

AZ_NODISCARD az_result
az_http_pipeline_process(az_http_request_builder * const hrb, const az_mut_span * const response) {
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);

  return pipeline[0].pfnc_process(&(pipeline[1]), hrb, response);
}
