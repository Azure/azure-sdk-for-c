// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_contract_internal.h>
#include <az_http.h>
#include <az_http_internal.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_http_pipeline_process(
    _az_http_pipeline * pipeline,
    _az_http_request * p_request,
    az_http_response * p_response) {
  AZ_CONTRACT_ARG_NOT_NULL(p_request);
  AZ_CONTRACT_ARG_NOT_NULL(p_response);
  AZ_CONTRACT_ARG_NOT_NULL(pipeline);

  return pipeline->_internal.p_policies[0]._internal.process(
      &(pipeline->_internal.p_policies[1]),
      pipeline->_internal.p_policies[0]._internal.p_options,
      p_request,
      p_response);
}
