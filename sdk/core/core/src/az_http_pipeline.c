// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_policy.h>

#include <az_contract.h>
#include <az_str.h>
#include <az_write_span_iter.h>

#include <_az_cfg.h>

///
/// HttpPipelineBuilder to add the pipeline policies to the request object
az_result az_http_pipeline_build(az_http_pipeline * p_pipeline) { return AZ_OK; };

az_result az_http_pipeline_next(az_http_pipeline const * self, az_http_request const * request) {
  if (self && self->pipeline_stage < self->num_policies) {
    self->pipeline_stage++;
    return self->policies[pipeline_stage](&self, &request);
  }
  return AZ_OK;
}

az_result az_http_pipeline_process(
    az_http_pipeline const * pipeline,
    az_http_request const * request) {
  return AZ_OK;
}

