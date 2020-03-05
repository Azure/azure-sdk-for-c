// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_POLICY_PRIVATE_H
#define _az_HTTP_POLICY_PRIVATE_H

#include <az_http_internal.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD AZ_INLINE az_result az_http_pipeline_nextpolicy(
    _az_http_policy* p_policies,
    _az_http_request* p_request,
    az_http_response* p_response)
{
  // Transport Policy is the last policy in the pipeline
  //  it returns without calling nextpolicy
  if (p_policies[0]._internal.process == NULL)
  {
    return AZ_ERROR_HTTP_PIPELINE_INVALID_POLICY;
  }

  return p_policies[0]._internal.process(
      &(p_policies[1]), p_policies[0]._internal.p_options, p_request, p_response);
}

#include <_az_cfg_suffix.h>

#endif // _az_HTTP_POLICY_PRIVATE_H
