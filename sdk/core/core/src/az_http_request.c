// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_request.h>

#include <az_contract.h>
#include <az_str.h>
#include <az_write_span_iter.h>

#include <_az_cfg.h>

#define AZ_CRLF "\r\n"

AZ_STATIC_ASSERT('\r' == 13)
AZ_STATIC_ASSERT('\n' == 10)

static az_const_span const az_crlf = AZ_CONST_STR(AZ_CRLF);

typedef struct {
  az_span_visitor visitor;
  az_const_span separator;
} az_data;

AZ_CALLBACK_DATA(az_data_to_pair_visitor, az_data *, az_pair_visitor)

az_result az_query_to_spans_func(az_data * const p, az_pair const pair) {
  AZ_CONTRACT_ARG_NOT_NULL(p);

  az_span_visitor const visitor = p->visitor;
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, p->separator));
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, pair.key));
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, AZ_STR("=")));
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, pair.value));
  p->separator = AZ_STR("&");
  return AZ_OK;
}

az_result az_header_to_spans_func(az_data * const p, az_pair const pair) {
  AZ_CONTRACT_ARG_NOT_NULL(p);

  az_span_visitor const visitor = p->visitor;
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, pair.key));
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, AZ_STR(": ")));
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, pair.value));
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, az_crlf));
  return AZ_OK;
}

az_result az_http_request_to_spans(
  az_http_request const* const p_request,
  az_span_visitor const visitor) {
  AZ_CONTRACT_ARG_NOT_NULL(p_request);

  az_data data = {
    .visitor = visitor,
    .separator = AZ_STR("?"),
  };

  // a request line
  {
    AZ_RETURN_IF_FAILED(visitor.func(visitor.data, p_request->method));
    AZ_RETURN_IF_FAILED(visitor.func(visitor.data, AZ_STR(" ")));
    AZ_RETURN_IF_FAILED(visitor.func(visitor.data, p_request->path));
    {
      az_pair_seq const query = p_request->query;
      az_pair_visitor const pair_visitor = az_data_to_pair_visitor(&data, az_query_to_spans_func);
      AZ_RETURN_IF_FAILED(query.func(query.data, pair_visitor));
    }
    AZ_RETURN_IF_FAILED(visitor.func(visitor.data, AZ_STR(" HTTP/1.1" AZ_CRLF)));
  }

  // headers
  {
    az_pair_seq const headers = p_request->headers;
    az_pair_visitor const pair_visitor = az_data_to_pair_visitor(&data, az_header_to_spans_func);
    AZ_RETURN_IF_FAILED(headers.func(headers.data, pair_visitor));
  }

  // empty line
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, az_crlf));

  // body.
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, p_request->body));

  return AZ_OK;
}

az_pair const az_http_standard_header
    = { .key = AZ_CONST_STR("ContentType"), .value = AZ_CONST_STR("text/plain; charset=utf-8") };

AZ_CALLBACK_DATA(az_http_standard_policy_to_pair_seq, az_http_standard_policy const *, az_pair_seq)

az_result az_http_standard_policy_func(
    az_http_standard_policy const * const p_data,
    az_pair_visitor const visitor) {
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, az_http_standard_header));
  {
    az_pair_seq const headers = p_data->headers;
    AZ_RETURN_IF_FAILED(headers.func(headers.data, visitor));
  }
  return AZ_OK;
}

az_result az_http_standard_policy_create(
    az_http_request * const p_request,
    az_http_standard_policy * const out) {
  out->headers = p_request->headers;
  p_request->headers = az_http_standard_policy_to_pair_seq(out, az_http_standard_policy_func);
  return AZ_OK;
}
