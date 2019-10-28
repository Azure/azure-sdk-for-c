// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_response.h>

#include <az_span_reader.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_http_response_parse(az_const_span const span, az_http_response const response) {
  az_span_reader reader = az_span_reader_create(span);

  // https://www.w3.org/Protocols/rfc2616/rfc2616-sec6.html
  // HTTP/1.1 200 OK
  //
}
