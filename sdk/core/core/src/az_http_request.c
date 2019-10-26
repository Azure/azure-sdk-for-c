// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_request.h>

#include <az_http_header.h>
#include <az_contract.h>
#include <az_span_builder.h>
#include <az_span_seq.h>
#include <az_str.h>

#include <stdlib.h>

#include <_az_cfg.h>

#define AZ_CRLF "\r\n"

static az_const_span const az_crlf = AZ_CONST_STR(AZ_CRLF);

/**
 * A query state.
 */
typedef struct {
  /**
   * A callback which accept a span of bytes.
   *
   * An immutable field.
   */
  az_span_append append;
  /**
   * A query parameter separator. Usually it's `&` but it can be `?` before the first query
   * parameter.
   *
   * A mutable field.
   */
  az_const_span separator;
} az_query_state;

AZ_CALLBACK_FUNC(az_build_query_param, az_query_state *, az_pair_append)

/**
 * Creates a span sequence from a query parameter.
 *
 * An example, if p->separator is "&", pair.key is "foo" and pair.value is "bar" then
 * the result sequence is "&foo=bar".
 *
 * Note: currently, the function assumes that pair.key and pair.value encoded into URL format.
 */
AZ_NODISCARD az_result az_build_query_param(az_query_state * const p, az_pair const pair) {
  AZ_CONTRACT_ARG_NOT_NULL(p);

  az_span_append const append = p->append;
  AZ_RETURN_IF_FAILED(az_span_append_do(append, p->separator));
  AZ_RETURN_IF_FAILED(az_span_append_do(append, pair.key));
  AZ_RETURN_IF_FAILED(az_span_append_do(append, AZ_STR("=")));
  AZ_RETURN_IF_FAILED(az_span_append_do(append, pair.value));
  p->separator = AZ_STR("&");
  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_header_line(az_span_append const * const p_append, az_pair const header) {
  AZ_RETURN_IF_FAILED(az_http_header_to_span_seq(&header, *p_append));
  AZ_RETURN_IF_FAILED(az_span_append_do(*p_append, az_crlf));
  return AZ_OK;
}

AZ_CALLBACK_FUNC(az_http_header_line, az_span_append const *, az_pair_append)

AZ_NODISCARD az_result
az_http_request_to_span_seq(az_http_request const * const p_request, az_span_append const append) {
  AZ_CONTRACT_ARG_NOT_NULL(p_request);

  // a request line
  {
    AZ_RETURN_IF_FAILED(az_span_append_do(append, p_request->method));
    AZ_RETURN_IF_FAILED(az_span_append_do(append, AZ_STR(" ")));
    AZ_RETURN_IF_FAILED(az_span_append_do(append, p_request->path));
    // query parameters
    {
      az_query_state state = {
        .append = append,
        .separator = AZ_STR("?"),
      };
      // for each query parameter apply `pair_visitor`
      AZ_RETURN_IF_FAILED(az_pair_seq_do(p_request->query, az_build_query_param_callback(&state)));
    }
    AZ_RETURN_IF_FAILED(az_span_append_do(append, AZ_STR(" HTTP/1.1" AZ_CRLF)));
  }

  // headers
  AZ_RETURN_IF_FAILED(az_pair_seq_do(p_request->headers, az_http_header_line_callback(&append)));

  // an empty line
  AZ_RETURN_IF_FAILED(az_span_append_do(append, az_crlf));

  // body.
  AZ_RETURN_IF_FAILED(az_span_append_do(append, p_request->body));

  return AZ_OK;
}

/*
AZ_NODISCARD az_result
az_build_url(az_http_request const * const p_request, az_span_append const append) {
  AZ_CONTRACT_ARG_NOT_NULL(p_request);

  // host path
  AZ_RETURN_IF_FAILED(az_span_append_do(append, p_request->path));
  // query parameters
  {
    az_query_state state = {
      .append = append,
      .separator = AZ_STR("?"),
    };
    // for each query parameter apply `az_query_to_spans`.
    AZ_RETURN_IF_FAILED(az_pair_seq_do(p_request->query, az_build_query_param_callback(&state)));
  }
  return AZ_OK;
}

AZ_CALLBACK_FUNC(az_build_url, az_http_request const *, az_span_seq)

AZ_NODISCARD az_result az_http_get_url_size(az_http_request const * const p_request, size_t * out) {
  return az_span_seq_size(az_build_url_callback(p_request), out);
}

AZ_NODISCARD az_result
az_http_url_to_new_str(az_http_request const * const p_request, char ** const out) {
  return az_span_seq_to_new_str(az_build_url_callback(p_request), out);
}
*/