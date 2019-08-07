// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_H
#define AZ_HTTP_H

#include "az_error.h"
#include "az_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  AZ_HTTP_METHOD_GET,
  AZ_HTTP_METHOD_HEAD,
  AZ_HTTP_METHOD_POST,
  AZ_HTTP_METHOD_PUT,
  AZ_HTTP_METHOD_DELETE,
  AZ_HTTP_METHOD_TRACE,
  AZ_HTTP_METHOD_OPTIONS,
  AZ_HTTP_METHOD_CONNECT,
  AZ_HTTP_METHOD_PATH,
} az_http_method;

az_error az_http_write_request_line(
  az_write_closure write,
  az_http_method method,
  az_cstr path);

#ifdef __cplusplus
}
#endif

#endif
