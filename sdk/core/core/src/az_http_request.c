// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_request.h>

#include <az_contract.h>
#include <az_span_builder.h>
#include <az_span_seq.h>
#include <az_str.h>

#include <stdlib.h>

#include <_az_cfg.h>

#define AZ_CRLF "\r\n"

static az_const_span const az_crlf = AZ_CONST_STR(AZ_CRLF);

typedef struct {
  az_span_visitor spans;
  az_const_span separator;
} az_query_state;

AZ_CALLBACK_FUNC(az_query_to_spans, az_query_state *, az_pair_visitor)

AZ_NODISCARD az_result az_query_to_spans(az_query_state * const p, az_pair const pair) {
  AZ_CONTRACT_ARG_NOT_NULL(p);

  az_span_visitor const spans = p->spans;
  AZ_RETURN_IF_FAILED(spans.func(spans.data, p->separator));
  AZ_RETURN_IF_FAILED(spans.func(spans.data, pair.key));
  AZ_RETURN_IF_FAILED(spans.func(spans.data, AZ_STR("=")));
  AZ_RETURN_IF_FAILED(spans.func(spans.data, pair.value));
  p->separator = AZ_STR("&");
  return AZ_OK;
}

AZ_NODISCARD az_result
az_build_header(az_pair const * const header, az_span_visitor const visitor) {
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, header->key));
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, AZ_STR(": ")));
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, header->value));
  return AZ_OK;
}

AZ_CALLBACK_FUNC(az_header_to_spans, az_span_visitor const *, az_pair_visitor)

AZ_NODISCARD az_result az_header_to_spans(az_span_visitor const * const p, az_pair const pair) {
  AZ_CONTRACT_ARG_NOT_NULL(p);

  AZ_RETURN_IF_FAILED(az_build_header(&pair, *p));
  AZ_RETURN_IF_FAILED(p->func(p->data, az_crlf));
  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_request_to_spans(
    az_http_request const * const p_request,
    az_span_visitor const spans) {
  AZ_CONTRACT_ARG_NOT_NULL(p_request);

  // a request line
  {
    AZ_RETURN_IF_FAILED(spans.func(spans.data, p_request->method));
    AZ_RETURN_IF_FAILED(spans.func(spans.data, AZ_STR(" ")));
    AZ_RETURN_IF_FAILED(spans.func(spans.data, p_request->path));
    // query parameters
    {
      az_query_state state = {
        .spans = spans,
        .separator = AZ_STR("?"),
      };
      az_pair_visitor const pair_visitor = az_query_to_spans_callback(&state);
      az_pair_seq const query = p_request->query;
      // for each query parameter apply `pair_visitor`
      AZ_RETURN_IF_FAILED(query.func(query.data, pair_visitor));
    }
    AZ_RETURN_IF_FAILED(spans.func(spans.data, AZ_STR(" HTTP/1.1" AZ_CRLF)));
  }

  // headers
  {
    az_pair_visitor const pair_visitor = az_header_to_spans_callback(&spans);
    az_pair_seq const headers = p_request->headers;
    AZ_RETURN_IF_FAILED(headers.func(headers.data, pair_visitor));
  }

  // an empty line
  AZ_RETURN_IF_FAILED(spans.func(spans.data, az_crlf));

  // body.
  AZ_RETURN_IF_FAILED(spans.func(spans.data, p_request->body));

  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_url_to_spans(
    az_http_request const * const p_request,
    az_span_visitor const spans) {
  AZ_CONTRACT_ARG_NOT_NULL(p_request);

  // host path
  {
    AZ_RETURN_IF_FAILED(spans.func(spans.data, p_request->path));
    // query parameters
    {
      az_query_state state = {
        .spans = spans,
        .separator = AZ_STR("?"),
      };
      az_pair_visitor const pair_visitor = az_query_to_spans_callback(&state);
      az_pair_seq const query = p_request->query;
      // for each query parameter apply `pair_visitor`
      AZ_RETURN_IF_FAILED(query.func(query.data, pair_visitor));
    }
  }
  return AZ_OK;
}

AZ_CALLBACK_FUNC(az_http_url_to_spans, az_http_request const *, az_span_seq)

AZ_NODISCARD az_result az_http_get_url_size(az_http_request const * const p_request, size_t * out) {
  return az_span_seq_size(az_http_url_to_spans_callback(p_request), out);
}

AZ_NODISCARD az_result az_http_url_to_new_str(az_http_request const * const p_request, char ** const out) {
  return az_span_seq_to_new_str(az_http_url_to_spans_callback(p_request), out);
}
