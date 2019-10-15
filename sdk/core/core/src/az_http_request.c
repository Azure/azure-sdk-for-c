// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_request.h>

#include <az_contract.h>
#include <az_str.h>
#include <az_write_span_iter.h>

#include <_az_cfg.h>

AZ_CALLBACK_DATA(az_pair_span_to_seq_data, az_pair_span const *, az_pair_seq)

az_result az_pair_span_to_seq_func(
    az_pair_span const * const context,
    az_pair_visitor const visitor) {
  AZ_CONTRACT_ARG_NOT_NULL(context);

  size_t const size = context->size;
  az_pair const * begin = context->begin;
  for (size_t i = 0; i < size; ++i) {
    AZ_RETURN_IF_FAILED(visitor.func(visitor.data, begin[i]));
  }
  return AZ_OK;
}

az_pair_seq az_pair_span_to_seq(az_pair_span const * const p_span) {
  return az_pair_span_to_seq_data(p_span, az_pair_span_to_seq_func);
}

#define AZ_CRLF "\r\n"

AZ_STATIC_ASSERT('\r' == 13)
AZ_STATIC_ASSERT('\n' == 10)

static az_const_span const az_crlf = AZ_CONST_STR(AZ_CRLF);

typedef struct {
  az_write_span_iter wi;
  az_const_span separator;
} az_data;

AZ_CALLBACK_DATA(az_data_to_pair_visitor, az_data *, az_pair_visitor)

az_result az_query_to_buffer_func(az_data * const p, az_pair const pair) {
  AZ_CONTRACT_ARG_NOT_NULL(p);

  AZ_RETURN_IF_FAILED(az_write_span_iter_write(&p->wi, p->separator));
  AZ_RETURN_IF_FAILED(az_write_span_iter_write(&p->wi, pair.key));
  AZ_RETURN_IF_FAILED(az_write_span_iter_write(&p->wi, AZ_STR("=")));
  AZ_RETURN_IF_FAILED(az_write_span_iter_write(&p->wi, pair.value));
  p->separator = AZ_STR("&");
  return AZ_OK;
}

az_result az_header_to_buffer_func(az_data * const p, az_pair const pair) {
  AZ_CONTRACT_ARG_NOT_NULL(p);

  AZ_RETURN_IF_FAILED(az_write_span_iter_write(&p->wi, pair.key));
  AZ_RETURN_IF_FAILED(az_write_span_iter_write(&p->wi, AZ_STR(": ")));
  AZ_RETURN_IF_FAILED(az_write_span_iter_write(&p->wi, pair.value));
  AZ_RETURN_IF_FAILED(az_write_span_iter_write(&p->wi, az_crlf));
  return AZ_OK;
}

az_result az_http_request_to_buffer(
    az_http_request const * const p_request,
    az_span const span,
    az_span * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_request);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  az_data data = {
    .wi = az_write_span_iter_create(span),
    .separator = AZ_STR("?"),
  };

  // a request line
  {
    AZ_RETURN_IF_FAILED(az_write_span_iter_write(&data.wi, p_request->method));
    AZ_RETURN_IF_FAILED(az_write_span_iter_write(&data.wi, AZ_STR(" ")));
    AZ_RETURN_IF_FAILED(az_write_span_iter_write(&data.wi, p_request->path));
    {
      az_pair_seq const query = p_request->query;
      az_pair_visitor const visitor = az_data_to_pair_visitor(&data, &az_query_to_buffer_func);
      AZ_RETURN_IF_FAILED(query.func(query.data, visitor));
    }
    AZ_RETURN_IF_FAILED(az_write_span_iter_write(&data.wi, AZ_STR(" HTTP/1.1" AZ_CRLF)));
  }

  // headers
  {
    az_pair_seq const headers = p_request->headers;
    az_pair_visitor const visitor = az_data_to_pair_visitor(&data, az_header_to_buffer_func);
    AZ_RETURN_IF_FAILED(headers.func(headers.data, visitor));
  }

  // empty line
  AZ_RETURN_IF_FAILED(az_write_span_iter_write(&data.wi, az_crlf));

  // body.
  AZ_RETURN_IF_FAILED(az_write_span_iter_write(&data.wi, p_request->body));

  *out = az_write_span_iter_result(&data.wi);

  return AZ_OK;
}

az_pair const az_http_standard_header
    = { .key = AZ_CONST_STR("ContentType"), .value = AZ_CONST_STR("text/plain; charset=utf-8") };

AZ_CALLBACK_DATA(az_http_standard_policy_to_pair_seq, az_http_standard_policy const *, az_pair_seq)

az_result az_http_standard_policy_func(
    az_http_standard_policy const * const p_data,
    az_pair_visitor const visitor) {
  az_pair_seq const headers = p_data->headers;
  AZ_RETURN_IF_FAILED(visitor.func(visitor.data, az_http_standard_header));
  AZ_RETURN_IF_FAILED(headers.func(headers.data, visitor));
  return AZ_OK;
}

az_result az_http_standard_policy_create(
    az_http_request * const p_request,
    az_http_standard_policy * const out) {
  out->headers = p_request->headers;
  p_request->headers = az_http_standard_policy_to_pair_seq(out, az_http_standard_policy_func);
  return AZ_OK;
}
