// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_PIPELINE_H
#define AZ_HTTP_PIPELINE_H

#include <az_contract.h>
#include <az_http_policy.h>
#include <az_http_request.h>
#include <az_pair.h>
#include <az_span.h>
#include <az_str.h>

#include <_az_cfg_prefix.h>
/*
struct az_http_policies * az_http_policy = { 
    { AZ_HTTP_POLICY_RETRY, null }, 
    { AZ_HTTP_POLICY_REQUESTID, null} 
};
*/

typedef struct {
  az_http_policy policies[10];
  //az_http_policy policies* az_http_policies;
} az_http_pipeline;

// Returns a pipeline
az_result az_http_pipeline_build(az_http_pipeline * p_pipeline);

// Start the pipeline
az_result az_http_pipeline_process(
    az_http_pipeline * const pipeline,
    az_http_request_builder * const hrb,
    az_http_response_data * const response);

#include <_az_cfg_suffix.h>
#endif