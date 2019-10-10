// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_REQUEST_H
#define AZ_HTTP_REQUEST_H

#include <az_contract.h>
#include <az_span.h>
#include <az_str.h>
#include <az_iter_data.h>
#include <az_pair.h>

#include <_az_cfg_prefix.h>

// request

typedef struct {
  az_const_span method;
  az_const_span path;
  az_pair_iter query;
  az_pair_iter headers;
  az_const_span body;
} az_http_request;

typedef az_result (*az_http_request_policy)(az_http_request *const p_policy);

az_result az_http_request_to_buffer(
    az_http_request const * const p_request,
    az_span const span,
    az_span * const out);

#include <_az_cfg_suffix.h>

#endif
