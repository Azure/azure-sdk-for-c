// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_policy.h>

#include <az_contract.h>
#include <az_str.h>

#include <az_http_pipeline.h>

#include <_az_cfg.h>

az_result az_http_pipeline_requestidpolicy(
    az_http_pipeline * const p_pipeline,
    az_http_request * const p_request,
    az_http_response_data * const out){
    //Append the Unique GUID into the headers
    //  x-ms-client-request-id
  return az_http_pipeline_process(p_pipeline, p_request, out);
}
