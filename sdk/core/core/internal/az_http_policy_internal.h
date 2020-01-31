// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_POLICY_INTERNAL_H
#define _az_HTTP_POLICY_INTERNAL_H

#include <az_http_policy.h>
#include <az_http_request_builder.h>
#include <az_http_response.h>
#include <az_result.h>

#include <_az_cfg_prefix.h>

typedef AZ_NODISCARD az_result (*az_http_pipeline_policy_func)(
    az_http_policy * p_policies,
    az_http_request_builder * hrb,
    az_http_response * response);

typedef struct {
  az_http_pipeline_policy_func const func;
  struct {
    az_http_policy * const p_policies;
    az_http_request_builder * const hrb;
    az_http_response * const response;
  } params;
} az_http_policy_callback;

#include <_az_cfg_suffix.h>

#endif
