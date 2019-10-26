// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_query.h>

#include <_az_cfg.h>

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
   * A query parameter separator. Usually it's `&` but it is `?` before the first query
   * parameter.
   *
   * A mutable field.
   */
  az_const_span separator;
} az_http_query_state;

AZ_CALLBACK_FUNC(az_http_query_param, az_http_query_state *, az_pair_append)

/**
 * Creates a span sequence from a query parameter.
 *
 * An example, if p->separator is "&", pair.key is "foo" and pair.value is "bar" then
 * the result sequence is "&foo=bar".
 *
 * Note: currently, the function assumes that pair.key and pair.value encoded into URL format.
 */
AZ_NODISCARD az_result az_http_query_param(az_http_query_state * const p, az_pair const pair) {
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
az_http_query_to_span_seq(az_pair_seq const query, az_span_append const append) {
  az_http_query_state state = {
    .append = append,
    .separator = AZ_STR("?"),
  };
  // for each query parameter apply `pair_visitor`
  AZ_RETURN_IF_FAILED(az_pair_seq_do(p_request->query, az_http_query_param_callback(&state)));
}
