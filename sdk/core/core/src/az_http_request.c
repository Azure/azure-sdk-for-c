// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_request.h>

#include <az_http_query.h>
#include <az_http_header.h>
#include <az_contract.h>
#include <az_span_builder.h>
#include <az_span_emitter.h>
#include <az_str.h>

#include <stdlib.h>

#include <_az_cfg.h>

static az_span const az_crlf = AZ_CONST_STR(AZ_CRLF);

AZ_NODISCARD az_result
az_http_header_line(az_write_span const * const self, az_pair const header) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_http_header_emit_span_seq(&header, *self));
  AZ_RETURN_IF_FAILED(az_write_span_do(*self, az_crlf));
  return AZ_OK;
}

AZ_ACTION_FUNC(az_http_header_line, az_write_span const, az_write_pair)

AZ_NODISCARD az_result
az_http_request_emit_span_seq(az_http_request const * const self, az_write_span const write_span) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  // a request line
  {
    AZ_RETURN_IF_FAILED(az_write_span_do(write_span, self->method));
    AZ_RETURN_IF_FAILED(az_write_span_do(write_span, AZ_STR(" ")));
    AZ_RETURN_IF_FAILED(az_write_span_do(write_span, self->path));
    AZ_RETURN_IF_FAILED(az_http_query_emit_span_seq(self->query, write_span));
    AZ_RETURN_IF_FAILED(az_write_span_do(write_span, AZ_STR(" HTTP/1.1" AZ_CRLF)));
  }

  // headers
  AZ_RETURN_IF_FAILED(az_pair_writer_do(self->headers, az_http_header_line_action(&write_span)));

  // an empty line
  AZ_RETURN_IF_FAILED(az_write_span_do(write_span, az_crlf));

  // body.
  AZ_RETURN_IF_FAILED(az_write_span_do(write_span, self->body));

  return AZ_OK;
}
