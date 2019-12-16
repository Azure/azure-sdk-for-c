// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_REQUEST_H
#define AZ_HTTP_REQUEST_H

#include <az_pair.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span method;
  az_span path;
  az_pair_writer query;
  az_pair_writer headers;
  az_span body;
} az_http_request;

/**
 * Creates a raw HTTP request.
 */
AZ_NODISCARD az_result
az_http_request_as_span_writer(az_http_request const * const self, az_span_action const write_span);

#include <_az_cfg_suffix.h>

#endif
