// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_policy.h>

#include <az_contract.h>
#include <az_str.h>
#include <az_write_span_iter.h>

#include <_az_cfg.h>

///
/// HttpPipelineBuilder to add the pipeline policies to the request object
az_result az_http_pipeline_build(az_http_request * p_request) { return AZ_OK; };

az_result az_http_pipeline_next(az_http_pipeline * self) {
  if (self && self->pipeline_stage < self->num_policies) {
    self->pipeline_stage++;
    return self->policies[pipeline_stage]();
  }
  return AZ_OK;
}
