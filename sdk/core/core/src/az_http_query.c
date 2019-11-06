// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_query.h>

#include <az_str.h>

#include <_az_cfg.h>

/**
 * A query state.
 */
typedef struct {
  /**
   * An action which accepts a span of bytes.
   *
   * An immutable field.
   */
  az_span_action span_action;
  /**
   * A query parameter separator. Usually it's `&` but it is `?` before the first query
   * parameter.
   *
   * A mutable field.
   */
  az_span separator;
} az_http_query_state;

AZ_ACTION_FUNC(az_http_query_param, az_http_query_state, az_pair_action)

/**
 * Creates a span sequence from a query parameter.
 *
 * An example, if p->separator is "&", pair.key is "foo" and pair.value is "bar" then
 * the result sequence is "&foo=bar".
 *
 * Note: currently, the function assumes that pair.key and pair.value encoded into URL format.
 */
AZ_NODISCARD az_result az_http_query_param(az_http_query_state * const p_state, az_pair const query_param) {
  AZ_CONTRACT_ARG_NOT_NULL(p_state);

  az_span_action const action = p_state->span_action;
  AZ_RETURN_IF_FAILED(az_span_action_do(action, p_state->separator));
  AZ_RETURN_IF_FAILED(az_span_action_do(action, query_param.key));
  AZ_RETURN_IF_FAILED(az_span_action_do(action, AZ_STR("=")));
  AZ_RETURN_IF_FAILED(az_span_action_do(action, query_param.value));
  p_state->separator = AZ_STR("&");
  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_query_emit_span_seq(az_pair_emitter const query, az_span_action const span_action) {
  az_http_query_state state = {
    .span_action = span_action,
    .separator = AZ_STR("?"),
  };
  return az_pair_emitter_do(query, az_http_query_param_action(&state));
}
