// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_REQUEST_H
#define AZ_HTTP_REQUEST_H

#include <az_contract.h>
#include <az_pair.h>
#include <az_span.h>
#include <az_str.h>

#include <_az_cfg_prefix.h>

#include <stdlib.h>

typedef struct {
  az_const_span method;
  az_const_span path;
  az_pair_seq query;
  az_pair_seq headers;
  az_const_span body;
} az_http_request;

az_result az_http_request_to_spans(
    az_http_request const * const p_request,
    az_span_visitor const span_visitor);

az_result az_http_url_to_spans(
    az_http_request const * const p_request,
    az_span_visitor const span_visitor);

az_result az_http_get_url_size(az_http_request const * const p_request, size_t * out);

az_result az_http_url_to_new_str(az_http_request const * const p_request, char ** const out);

az_result az_build_header(az_pair const * header, az_span_visitor const visitor);

#include <_az_cfg_suffix.h>

#endif
