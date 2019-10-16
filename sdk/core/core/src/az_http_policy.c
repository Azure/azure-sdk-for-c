// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_policy.h>

#include <az_contract.h>
#include <az_str.h>
#include <az_write_span_iter.h>

#include <_az_cfg.h>

az_result az_http_pipeline_requestidpolicy(
    az_http_request * const p_request,
    az_http_response_data * const out){
    //Append the Unique GUID into the headers
    //  x-ms-client-request-id
  return AZ_OK;
}
