// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_pipeline.h>
#include <az_http_policy.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_http_policies_init(az_http_policies * const self)
{
  AZ_CONTRACT_ARG_NOT_NULL(self);

  self->uniquerequestid.pfnc_process = az_http_pipeline_policy_uniquerequestid;
  self->retry.pfnc_process = az_http_pipeline_policy_retry;
  self->authentication.pfnc_process = az_http_pipeline_policy_authentication;
  self->logging.pfnc_process = az_http_pipeline_policy_logging;
  self->bufferresponse.pfnc_process = az_http_pipeline_policy_bufferresponse;
  self->distributedtracing.pfnc_process = az_http_pipeline_policy_distributedtracing;
  self->transport.pfnc_process = az_http_pipeline_policy_transport;

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_pipeline_process(
    az_http_request_builder * const hrb,
    const az_mut_span * const response,
    az_http_policies const * const policies) {
  AZ_CONTRACT_ARG_NOT_NULL(hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);
  AZ_CONTRACT_ARG_NOT_NULL(policies);

  az_http_policy pipeline[] = {
    { .pfnc_process = policies->uniquerequestid.pfnc_process,
      .data = policies->uniquerequestid.data },
    { .pfnc_process = policies->retry.pfnc_process, .data = policies->retry.data },
    { .pfnc_process = policies->authentication.pfnc_process,
      .data = policies->authentication.data },
    { .pfnc_process = policies->logging.pfnc_process, .data = policies->logging.data },
    { .pfnc_process = policies->bufferresponse.pfnc_process,
      .data = policies->bufferresponse.data },
    { .pfnc_process = policies->distributedtracing.pfnc_process,
      .data = policies->distributedtracing.data },
    { .pfnc_process = policies->transport.pfnc_process, .data = policies->transport.data },
    { .pfnc_process = NULL, .data = NULL },
  };

  return pipeline[0].pfnc_process(&(pipeline[1]), pipeline[0].data, hrb, response);
}
