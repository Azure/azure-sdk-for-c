// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_RESPONSE_H
#define AZ_HTTP_RESPONSE_H

#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span_visitor header_name;
  az_span_visitor header_value;
  az_span_visitor body;
} az_http_response;

AZ_NODISCARD az_result az_http_response_parse(az_const_span const buffer, az_http_response const response);

#include <_az_cfg_suffix.h>

#endif
