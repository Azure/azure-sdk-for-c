// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_pipeline.h>
#include <az_http_policy.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_http_pipeline_process(
    az_http_pipeline * pipeline,
    az_http_request_builder * const hrb,
    az_http_response const * const response) {
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);
  AZ_CONTRACT_ARG_NOT_NULL(pipeline);

  return pipeline->policies[0].pfnc_process(
      &(pipeline->policies[1]), pipeline->policies[0].data, hrb, response);
}
