// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_contract_internal.h>
#include <az_http.h>
#include <az_http_pipeline_internal.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_http_pipeline_process(
    az_http_pipeline * pipeline,
    az_http_request * p_request,
    az_http_response * p_response) {
  AZ_CONTRACT_ARG_NOT_NULL(p_request);
  AZ_CONTRACT_ARG_NOT_NULL(p_response);
  AZ_CONTRACT_ARG_NOT_NULL(pipeline);

  return pipeline->p_policies[0].process(
      &(pipeline->p_policies[1]), pipeline->p_policies[0].p_options, p_request, p_response);
}
