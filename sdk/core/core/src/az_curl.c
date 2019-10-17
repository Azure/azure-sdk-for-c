// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_curl.h>
#include <az_write_span_iter.h>

#include <stdlib.h>

typedef struct {
  struct curl_slist * p_list;
} az_headers_data;

AZ_CALLBACK_DATA(az_create_headers_callback, az_headers_data *, az_pair_visitor)

az_result az_headers_to_curl(az_headers_data * const p_state, az_pair const pair) {
  az_const_span const header_tokens[] = {
    pair.key,
    AZ_STR(": "),
    pair.value,
  };
  az_span_span const tokens_span = AZ_SPAN(header_tokens);
  az_span_seq const tokens_seq = az_span_span_to_seq(&tokens_span);

  char * str_header;
  AZ_RETURN_IF_FAILED(az_span_seq_to_new_str(tokens_seq, &str_header));

  printf("\nSet header: %s", str_header);

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

az_result az_curl_http_request(az_curl * const p_curl, az_http_request const * const p_request) {
  // creates a slist for bulding curl headers
  az_headers_data headers = {
    .p_list = NULL,
  };
  // build headers into a slist as curl is expecting
  az_build_headers(p_request, &headers);
  // set all headers from slist
  curl_easy_setopt(p_curl->p_curl, CURLOPT_HTTPHEADER, headers.p_list);

  // build URL with query params
  // az_span_visitor sv = az_write_span_iter_to_span_visitor(&wi);
  // AZ_RETURN_IF_FAILED(az_http_request_to_url_span(p_request, sv));

  // TODO: would this be part of done function instead?
  curl_slist_free_all(headers.p_list);

  return AZ_OK;
}
