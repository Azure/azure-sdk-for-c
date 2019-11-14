// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_pipeline.h>
#include <az_http_policy.h>

#include <_az_cfg.h>

az_http_policy pipeline[] = {
  { .pfnc_process = az_http_pipeline_policy_uniquerequestid },
  { .pfnc_process = az_http_pipeline_policy_retry, },
  { .pfnc_process = az_http_pipeline_policy_authentication, },
  { .pfnc_process = az_http_pipeline_policy_logging, },
  { .pfnc_process = az_http_pipeline_policy_bufferresponse, },
  { .pfnc_process = az_http_pipeline_policy_distributedtracing, },
  { .pfnc_process = az_http_pipeline_policy_transport, },
  { .pfnc_process = NULL, },
};

AZ_NODISCARD az_result
az_http_pipeline_process(az_http_policy_arg * const arg) {
  AZ_CONTRACT_ARG_NOT_NULL(arg);

  return pipeline->pfnc_process(pipeline + 1, arg);
}
