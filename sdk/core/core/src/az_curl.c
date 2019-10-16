// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_callback.h>
#include <az_curl.h>
#include <az_span_seq.h>

#include <_az_cfg.h>

typedef struct {
  struct curl_slist * p_list;
} az_headers_data;

AZ_CALLBACK_DATA(az_create_headers_callback, az_headers_data *, az_pair_visitor)

az_result az_headers_to_curl(az_headers_data * const p_state, az_pair const pair) {
  printf("\nSet header: %s:%s", pair.key.begin, pair.value.begin);
  az_const_span const header[3] = {
    pair.key,
    AZ_STR(": "),
    pair.value,
  };
  az_span_span const span = AZ_SPAN(header);
  az_span_seq const seq = az_span_span_to_seq(&span);
  size_t size;
  AZ_RETURN_IF_FAILED(az_span_seq_size(seq, &size));
  p_state->p_list = curl_slist_append(p_state->p_list, "TODO: JOIN_KEY_AND_VALUE_HERE");
  return AZ_OK;
}

az_result az_build_headers(az_http_request const * const p_request, az_headers_data * headers) {
  // create callback for visitor
  az_pair_visitor const pair_visitor = az_create_headers_callback(headers, az_headers_to_curl);

  az_pair_seq const request_headers_seq = p_request->headers;
  AZ_RETURN_IF_FAILED(request_headers_seq.func(request_headers_seq.data, pair_visitor));

  return AZ_OK;
}

az_result az_curl_http_request(az_curl * const p_curl, az_http_request const * const p_request) {
  // creates a slist for bulding curl headers
  az_headers_data headers = {
    .p_list = NULL,
  };

  az_build_headers(p_request, &headers);

  curl_easy_setopt(p_curl->p_curl, CURLOPT_HTTPHEADER, headers.p_list);

  // TODO: would this be part of done function instead?
  curl_slist_free_all(headers.p_list);

  return AZ_OK;
}
