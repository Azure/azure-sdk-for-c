// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_RESPONSE_H
#define AZ_HTTP_RESPONSE_H

#include <az_pair.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef struct {
  /**
   * Note: if we recieve data in chunks, we should have two `span_action`. One for a haeder name and
   * another one for a header value. Same design change may apply on JSON parser.
   */
  az_pair_action header;
  az_span_action body;
} az_http_response;

AZ_NODISCARD az_result
az_http_response_parse(az_span const buffer, az_http_response const response);

#include <_az_cfg_suffix.h>

#endif
