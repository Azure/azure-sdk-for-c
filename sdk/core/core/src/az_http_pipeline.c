// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_pipeline.h>
#include <az_http_policy.h>

#include <_az_cfg.h>

static az_http_policy pipeline[] = {
  { .pfnc_process = az_http_pipeline_policy_uniquerequestid, .data = { 0 } },
  { .pfnc_process = az_http_pipeline_policy_retry, .data = { 0 } },
  { .pfnc_process = az_http_pipeline_policy_authentication, .data = { 0 } },
  { .pfnc_process = az_http_pipeline_policy_logging, .data = { 0 } },
  { .pfnc_process = az_http_pipeline_policy_bufferresponse, .data = { 0 } },
  { .pfnc_process = az_http_pipeline_policy_distributedtracing, .data = { 0 } },
  { .pfnc_process = az_http_pipeline_policy_transport, .data = { 0 } },
  { .pfnc_process = NULL, .data = { 0 } },
};

AZ_NODISCARD az_result az_http_pipeline_process(
    az_http_request_builder * const hrb,
    const az_mut_span * const response,
    az_http_policy_data * const policy_data) {
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);

  // FIXME global data
  // Request auth instead of policy_data, and only set .credentials for the auth policy then?
  // Not sure how this wassupposed to work, check with Rick.
  for (size_t i = 0; i < sizeof(pipeline) / sizeof(pipeline[0]) - 1; ++i) {
    pipeline[i].data = *policy_data;
  }

  return pipeline[0].pfnc_process(&(pipeline[1]), hrb, response);
}
