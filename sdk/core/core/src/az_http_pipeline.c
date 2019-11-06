// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT


#include <az_http_pipeline.h>
#include <az_http_policy.h>

#include <az_contract.h>
#include <az_str.h>


#include <_az_cfg.h>

///
/// HttpPipelineBuilder to add the pipeline policies to the request object
az_result az_http_pipeline_build(az_http_pipeline * p_pipeline) {
  AZ_CONTRACT_ARG_NOT_NULL(p_pipeline);
  return AZ_OK; 
}

az_result az_http_pipeline_process(
    az_http_pipeline * const self,
    az_http_request_builder * const hrb,
    az_http_response_data * const response) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);

  if (self) {
    return self->policies[0].pfnc_process(self->policies, hrb, response);
  }
  return AZ_OK;
}