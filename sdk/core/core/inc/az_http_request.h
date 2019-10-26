// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_REQUEST_H
#define AZ_HTTP_REQUEST_H

#include <az_pair.h>
#include <az_span.h>
#include <az_span_seq.h>
#include <az_str.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_const_span method;
  az_const_span path;
  az_pair_seq query;
  az_pair_seq headers;
  az_const_span body;
} az_http_request;

AZ_NODISCARD az_result
az_http_request_to_span_seq(az_http_request const * const p_request, az_span_append const append);

// AZ_NODISCARD az_result
// az_build_url(az_http_request const * const p_request, az_span_append const append);

// AZ_NODISCARD az_result az_http_get_url_size(az_http_request const * const p_request, size_t *
// out);

// AZ_NODISCARD az_result
// az_http_url_to_new_str(az_http_request const * const p_request, char ** const out);

// AZ_CALLBACK_FUNC(az_build_header, az_pair const *, az_span_seq)

#include <_az_cfg_suffix.h>

#endif
