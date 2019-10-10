// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_request.h>

#include <az_contract.h>
#include <az_str.h>

#include <_az_cfg_warn.h>

typedef struct {
  az_span span;
  size_t i;
} az_write_span_iter;

static inline az_write_span_iter az_write_span_iter_create(az_span const span) {
  return (az_write_span_iter){
    .span = span,
    .i = 0,
  };
}

static inline az_span az_write_span_iter_result(az_write_span_iter const * const p_i) {
  return az_span_take(p_i->span, p_i->i);
}

static inline az_result az_write_span_iter_write(
    az_write_span_iter * const p_i,
    az_const_span const span) {
  AZ_CONTRACT_ARG_NOT_NULL(p_i);

  az_span const remainder = az_span_drop(p_i->span, p_i->i);
  AZ_RETURN_IF_FAILED(az_span_copy(span, remainder));
  p_i->i += span.size;
  return AZ_OK;
}

#define AZ_CRLF "\r\n"

AZ_STATIC_ASSERT('\r' == 13)
AZ_STATIC_ASSERT('\n' == 10)

static az_const_span const az_crlf = AZ_CONST_STR(AZ_CRLF);

az_result az_http_request_to_buffer(
    az_http_request const * const p_request,
    az_span const span,
    az_span * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(p_request);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  az_write_span_iter wi = az_write_span_iter_create(span);

  // a request line
  {
    AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, p_request->method));
    AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, AZ_STR(" ")));
    AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, p_request->path));
    // query parameters
    {
      az_const_span separator = AZ_STR("?");
      az_pair_iter i = p_request->query;
      while (true) {
        az_pair p;
        {
          az_result const result = az_pair_iter_call(&i, &p);
          if (result == AZ_ERROR_EOF) {
            break;
          }
          AZ_RETURN_IF_FAILED(result);
        }
        AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, separator));
        AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, p.key));
        AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, AZ_STR("=")));
        AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, p.value));
        separator = AZ_STR("&");
      }
    }
    AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, AZ_STR(" HTTP/1.1" AZ_CRLF)));
  }

  // headers
  {
    az_pair_iter i = p_request->headers;
    while (true) {
      az_pair p;
      {
        az_result const result = az_pair_iter_call(&i, &p);
        if (result == AZ_ERROR_EOF) {
          break;
        }
        AZ_RETURN_IF_FAILED(result);
      }
      AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, p.key));
      AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, AZ_STR(": ")));
      AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, p.value));
      AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, az_crlf));
    }
  }

  // empty line
  AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, az_crlf));

  // body.
  AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, p_request->body));

  *out = az_write_span_iter_result(&wi);

  return AZ_OK;
}

static az_pair const az_http_standard_headers_array[] = {
  { .key = AZ_CONST_STR("ContentType"), .value = AZ_CONST_STR("text/plain; charset=utf-8") },
};

az_result az_http_standard_headers_iter_func(az_pair_iter * const p_iter, az_pair * const out) {
  size_t const size = AZ_ARRAY_SIZE(az_http_standard_headers_array);
  size_t const i = (size_t)(p_iter->data.end);
  if (i < size) {
    *out = az_http_standard_headers_array[i];
    p_iter->data.end = (void const *)(i + 1);
    return AZ_OK;
  }
  az_http_standard_headers const * h = p_iter->data.begin;
  *p_iter = h->original_headers;
  return AZ_OK;
}

az_result az_http_standard_headers_policy(
    az_http_request const * const p_request,
    az_http_standard_headers * const out) {
  *out = (az_http_standard_headers){
    .original_headers = p_request->headers,
    .request = *p_request,
  };
  out->request.headers = (az_pair_iter){
    .func = az_http_standard_headers_iter_func,
    .data = { .begin = out, .end = 0, },
  };
  return AZ_OK;
}
