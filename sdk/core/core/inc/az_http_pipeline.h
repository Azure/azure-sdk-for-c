// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_PIPELINE_H
#define AZ_HTTP_PIPELINE_H

#include <az_contract.h>
#include <az_http_request.h>
#include <az_iter_data.h>
#include <az_pair.h>
#include <az_span.h>
#include <az_str.h>

#include <az_http_policy.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_http_policy policies[10];
  int num_policies;
  int pipeline_stage;

  //Each policy calls next
  az_result (*next)(az_http_pipeline *pipeline);
} az_http_pipeline;

#include <_az_cfg_suffix.h>

#endif