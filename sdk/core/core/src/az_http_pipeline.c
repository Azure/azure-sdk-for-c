// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_pipeline.h>
#include <az_http_policy.h>

#include <az_contract.h>
#include <az_str.h>

#include <_az_cfg.h>

az_http_policy pipeline[8] = {
  { .pfnc_process = az_http_pipeline_policy_uniquerequestid, .data = NULL },
  { .pfnc_process = az_http_pipeline_policy_retry, .data = NULL },
  { .pfnc_process = az_http_pipeline_policy_authentication, .data = NULL },
  { .pfnc_process = az_http_pipeline_policy_logging, .data = NULL },
  { .pfnc_process = az_http_pipeline_policy_bufferresponse, .data = NULL },
  { .pfnc_process = az_http_pipeline_policy_distributedtracing, .data = NULL },
  { .pfnc_process = az_http_pipeline_policy_transport, .data = NULL },
  NULL,
};

az_result az_http_pipeline_process(
    az_http_request_builder * const hrb,
    az_mut_span const * const response) {
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);

  return pipeline[0].pfnc_process(&(pipeline[1]), hrb, response);
}