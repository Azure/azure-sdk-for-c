// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_curl_adapter.h>

#include <_az_cfg.h>

typedef struct {
  struct curl_slist * p_list;
} az_headers_data;

/*
AZ_CALLBACK_DATA(az_create_headers_callback, az_headers_data *, az_pair_visitor)
AZ_CALLBACK_DATA(az_pair_callback, az_pair const *, az_span_seq)

az_result az_headers_to_curl(az_headers_data * const p_state, az_pair const header) {
  const az_span_seq token_seq = az_pair_callback(&header, az_build_header);
  char * str_header;
  AZ_RETURN_IF_FAILED(az_span_seq_to_new_str(token_seq, &str_header));

  p_state->p_list = curl_slist_append(p_state->p_list, str_header);

  free(str_header);
  return AZ_OK;
}

az_result az_build_headers(az_http_request const * const p_request, az_headers_data * headers) {
  // create callback for visitor
  az_pair_visitor const pair_visitor = az_create_headers_callback(headers, az_headers_to_curl);

  az_pair_seq const request_headers_seq = p_request->headers;
  AZ_RETURN_IF_FAILED(request_headers_seq.func(request_headers_seq.data, pair_visitor));

  return AZ_OK;
}
*/

az_result az_http_client_send_request_impl(
    az_http_request_builder const * const p_hrb,
    az_span * const response) {
  return AZ_OK;
};
