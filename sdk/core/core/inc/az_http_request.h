// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_REQUEST_H
#define AZ_HTTP_REQUEST_H

#include <az_pair.h>
#include <az_span.h>
#include <az_span_emitter.h>
#include <az_str.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_const_span method;
  az_const_span path;
  az_pair_emitter query;
  az_pair_emitter headers;
  az_const_span body;
} az_http_request;

/**
 * Creates a raw HTTP request.
 */
AZ_NODISCARD az_result
az_http_request_emit_span_seq(az_http_request const * const self, az_span_action const action);

#include <_az_cfg_suffix.h>

#endif
