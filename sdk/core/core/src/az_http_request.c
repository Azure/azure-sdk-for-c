// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_request_internal.h>

#include <az_contract_internal.h>
#include <az_http_header_internal.h>
#include <az_http_query_internal.h>

#include "az_str_private.h"
#include <az_span_writer_internal.h>

#include <stdlib.h>

#include <_az_cfg.h>

static az_span const az_crlf = AZ_SPAN_LITERAL_FROM_STR(AZ_CRLF);

AZ_NODISCARD az_result
az_http_header_line(az_span_action write_span, az_pair header, az_span_action * out) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  AZ_RETURN_IF_FAILED(az_http_header_as_span_writer(header, write_span, &header));
  AZ_RETURN_IF_FAILED(az_span_action_do(write_span, az_crlf));
  return AZ_OK;
}

AZ_ACTION_FUNC(az_http_header_line, az_span_action, az_pair_action)

AZ_NODISCARD az_result az_http_request_as_span_writer(
    az_http_request self,
    az_span_action write_span,
    az_http_request * out) {
  AZ_CONTRACT_ARG_NOT_NULL(out);
  (void)self;
  // a request line
  {
    AZ_RETURN_IF_FAILED(az_span_action_do(write_span, out->method));
    AZ_RETURN_IF_FAILED(az_span_action_do(write_span, AZ_SPAN_FROM_STR(" ")));
    AZ_RETURN_IF_FAILED(az_span_action_do(write_span, out->path));
    AZ_RETURN_IF_FAILED(az_http_query_as_span_writer(out->query, write_span));
    AZ_RETURN_IF_FAILED(az_span_action_do(write_span, AZ_SPAN_FROM_STR(" HTTP/1.1" AZ_CRLF)));
  }

  // headers
  AZ_RETURN_IF_FAILED(az_pair_writer_do(out->headers, az_http_header_line_action(&write_span)));

  // an empty line
  AZ_RETURN_IF_FAILED(az_span_action_do(write_span, az_crlf));

  // body.
  AZ_RETURN_IF_FAILED(az_span_action_do(write_span, out->body));

  return AZ_OK;
}
