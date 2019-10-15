// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_REQUEST_H
#define AZ_HTTP_REQUEST_H

#include <az_contract.h>
#include <az_pair.h>
#include <az_span.h>
#include <az_str.h>

#include <_az_cfg_prefix.h>

// request

typedef struct {
  az_const_span method;
  az_const_span path;
  az_pair_seq query;
  az_pair_seq headers;
  az_const_span body;
} az_http_request;

az_result az_http_request_to_buffer(
    az_http_request const * const p_request,
    az_span const span,
    az_span * const out);

typedef struct {
  az_pair_seq headers;
} az_http_standard_policy;

/**
 * Note: `*p_request` should not be used after `*out` is destroyed.
 */
az_result az_http_standard_policy_create(
    az_http_request * const p_request,
    az_http_standard_policy * const out);

#include <_az_cfg_suffix.h>

#endif
