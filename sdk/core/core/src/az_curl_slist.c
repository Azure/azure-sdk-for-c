// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_curl_slist.h>

#include <az_action.h>
#include <az_http_header.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_curl_slist_append(struct curl_slist ** const self, char const * const str) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(str);

  struct curl_slist * const p_list = curl_slist_append(*self, str);
  if (p_list == NULL) {
    return AZ_ERROR_HTTP_PAL;
  }
  *self = p_list;
  return AZ_OK;
}

/**
 * Creates a function `az_curl_slist_append_action(struct curl_slist **)`
 * that returns a value of type `az_str_action`.
 */
AZ_ACTION_FUNC(az_curl_slist_append, struct curl_slist *, az_str_action)

AZ_NODISCARD az_result
az_curl_slist_append_header(struct curl_slist ** const self, az_pair const header) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_span_emitter const header_span_emitter = az_http_header_emit_span_seq_action(&header);
  az_str_action const curl_slist_append_action = az_curl_slist_append_action(self);

  // the function creates a temporary dynamic zero-terminated string from `header_span_emitter`
  // and passes it to `curl_slist_append_action`.
  return az_span_emitter_to_tmp_str(header_span_emitter, curl_slist_append_action);
}
