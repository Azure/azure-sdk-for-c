// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_NET_H
#define AZ_NET_H

#include <az_result.h>
#include <az_span.h>

#include <stddef.h>

#include <_az_cfg_prefix.h>

typedef enum {
  AZ_NET_HTTP_METHOD_POST = 0,
  AZ_NET_HTTP_METHOD_GET = 1,
} az_net_http_method;

az_result az_net_invoke_rest_method(
    az_net_http_method const http_method,
    az_const_span const uri,
    az_const_span const body,
    az_const_span const headers,
    az_span const response_buffer,
    az_const_span * const out_response);

az_result az_net_uri_escape(
    az_const_span const s,
    az_span const buffer,
    az_const_span * const out_result);

static inline az_result az_net_uri_unescape(
    az_const_span const s,
    az_span const buffer,
    az_const_span * const out_result) {
  if (out_result == NULL) {
    return AZ_ERROR_ARG;
  }

  {
    // C4100: unreferenced formal parameter && error: statement with no effect
    assert(&s != NULL);
    assert(&buffer != NULL);
  }

  return AZ_ERROR_NOT_IMPLEMENTED;
}

#include <_az_cfg_suffix.h>

#endif
