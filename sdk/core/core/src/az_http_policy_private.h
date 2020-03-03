// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_POLICY_PRIVATE_H
#define _az_HTTP_POLICY_PRIVATE_H

#include <az_http.h>
#include <az_result.h>

#include <_az_cfg_prefix.h>

typedef AZ_NODISCARD az_result (*_az_http_pipeline_policy_fn)(
    _az_http_policy* policies,
    _az_http_request* ref_request,
    az_http_response* ref_response);

typedef struct
{
  _az_http_pipeline_policy_fn const func;
  struct
  {
    _az_http_policy* policies;
    _az_http_request* ref_request;
    az_http_response* ref_response;
  } params;
} az_http_policy_callback;

#include <_az_cfg_suffix.h>

#endif // _az_HTTP_POLICY_PRIVATE_H
