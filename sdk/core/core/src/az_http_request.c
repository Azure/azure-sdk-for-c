// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_request.h>

#include <az_contract.h>
#include <az_span_seq.h>
#include <az_str.h>
#include <az_write_span_iter.h>

#include <_az_cfg.h>

#define AZ_CRLF "\r\n"

AZ_STATIC_ASSERT('\r' == 13)
AZ_STATIC_ASSERT('\n' == 10)

static az_const_span const az_crlf = AZ_CONST_STR(AZ_CRLF);

typedef struct {
  az_span_visitor spans;
  az_const_span separator;
} az_query_state;

// AZ_CALLBACK_DATA(az_query_state_to_pair_visitor, az_query_state *, az_pair_visitor)

AZ_CALLBACK_FUNC(az_query_to_spans, az_query_state *, az_pair_visitor)

az_result az_query_to_spans(az_query_state * const p, az_pair const pair) {
  AZ_CONTRACT_ARG_NOT_NULL(p);

  az_span_visitor const spans = p->spans;
  AZ_RETURN_IF_FAILED(spans.func(spans.data, p->separator));
  AZ_RETURN_IF_FAILED(spans.func(spans.data, pair.key));
  AZ_RETURN_IF_FAILED(spans.func(spans.data, AZ_STR("=")));
  AZ_RETURN_IF_FAILED(spans.func(spans.data, pair.value));
  p->separator = AZ_STR("&");
  return AZ_OK;
}

az_result az_build_header(az_pair const * header, az_span_visitor const visitor) {
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, header->key));
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, AZ_STR(": ")));
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, header->value));
  return AZ_OK;
}

// AZ_CALLBACK_DATA(az_span_visitor_to_pair_visitor, az_span_visitor const *, az_pair_visitor)

AZ_CALLBACK_FUNC(az_header_to_spans, az_span_visitor const *, az_pair_visitor)

az_result az_header_to_spans(az_span_visitor const * const p, az_pair const pair) {
  AZ_CONTRACT_ARG_NOT_NULL(p);

  AZ_RETURN_IF_FAILED(az_build_header(&pair, *p));
  AZ_RETURN_IF_FAILED(p->func(p->data, az_crlf));
  return AZ_OK;
}

az_result az_http_request_to_spans(
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

az_result az_http_url_to_spans(
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

az_result az_http_get_url_size(az_http_request const * const p_request, size_t * out) {
  return az_http_url_to_spans(p_request, az_span_add_size_callback(out));
}

az_result az_http_url_to_new_str(az_http_request const * const p_request, char ** const out) {
  *out = NULL;
  size_t size = 0;
  AZ_RETURN_IF_FAILED(az_http_get_url_size(p_request, &size));
  size += 1;
  uint8_t * const p = (uint8_t *)malloc(size);
  if (p == NULL) {
    return AZ_ERROR_OUT_OF_MEMORY;
  }
  az_write_span_iter i = az_write_span_iter_create((az_span){ .begin = p, .size = size });
  az_span_visitor sv = az_write_span_iter_write_callback(&i);
  az_result const result = az_http_url_to_spans(p_request, sv);
  az_write_span_iter_write(&i, AZ_STR("\0"));
  if (az_failed(result)) {
    free(p);
    return result;
  }
  *out = (char *)p;
  return AZ_OK;
}
