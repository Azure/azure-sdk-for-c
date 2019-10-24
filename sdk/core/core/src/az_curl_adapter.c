// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_curl_adapter.h>

#include <az_callback.h>
#include <az_http_request.h>

#include <_az_cfg.h>

typedef struct {
  struct curl_slist * p_list;
} az_headers_data;

AZ_CALLBACK_FUNC(az_headers_to_curl, az_headers_data *, az_pair_visitor)

az_result az_headers_to_curl(az_headers_data * const p_state, az_pair const header) {
  const az_span_seq token_seq = az_build_header_callback(&header);
  char * str_header;
  AZ_RETURN_IF_FAILED(az_span_seq_to_new_str(token_seq, &str_header));

  p_state->p_list = curl_slist_append(p_state->p_list, str_header);

  free(str_header);
  return AZ_OK;
}

az_result az_build_headers(az_http_request const * const p_request, az_headers_data * p_headers) {
  // create callback for visitor
  az_pair_visitor const pair_visitor = az_headers_to_curl_callback(p_headers);

  az_pair_seq const request_headers_seq = p_request->headers;
  AZ_RETURN_IF_FAILED(request_headers_seq.func(request_headers_seq.data, pair_visitor));

  return AZ_OK;
}
