// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "../inc/internal/az_http_request.h"
#include "../inc/internal/_az_span_builder.h"
#include "../inc/internal/az_http_header.h"
#include "../inc/internal/az_http_query.h"
#include <az_span_builder.h>
#include <az_span_writer.h>
#include <az_str.h>

#include <stdlib.h>

#include <_az_cfg.h>

static az_span const az_crlf = AZ_CONST_STR(AZ_CRLF);

AZ_NODISCARD az_result
az_http_header_line(az_span_action const * const write_span, az_pair const header) {
  AZ_CONTRACT_ARG_NOT_NULL(write_span);

  AZ_RETURN_IF_FAILED(az_http_header_as_span_writer(&header, *write_span));
  AZ_RETURN_IF_FAILED(az_span_action_do(*write_span, az_crlf));
  return AZ_OK;
}

AZ_ACTION_FUNC(az_http_header_line, az_span_action const, az_pair_action)

AZ_NODISCARD az_result az_http_request_as_span_writer(
    az_http_request const * const self,
    az_span_action const write_span) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  // a request line
  {
    AZ_RETURN_IF_FAILED(az_span_action_do(write_span, self->method));
    AZ_RETURN_IF_FAILED(az_span_action_do(write_span, AZ_STR(" ")));
    AZ_RETURN_IF_FAILED(az_span_action_do(write_span, self->path));
    AZ_RETURN_IF_FAILED(az_http_query_as_span_writer(self->query, write_span));
    AZ_RETURN_IF_FAILED(az_span_action_do(write_span, AZ_STR(" HTTP/1.1" AZ_CRLF)));
  }

  // headers
  AZ_RETURN_IF_FAILED(az_pair_writer_do(self->headers, az_http_header_line_action(&write_span)));

  // an empty line
  AZ_RETURN_IF_FAILED(az_span_action_do(write_span, az_crlf));

  // body.
  AZ_RETURN_IF_FAILED(az_span_action_do(write_span, self->body));

  return AZ_OK;
}
