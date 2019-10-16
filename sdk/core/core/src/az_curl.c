// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_callback.h>
#include <az_curl.h>

#include <_az_cfg.h>

typedef struct {
  struct curl_slist * p_list;
} az_headers_data;

AZ_CALLBACK_DATA(az_create_headers_callback, az_headers_data *, az_pair_visitor)

az_result az_headers_to_curl(az_headers_data * const p_state, az_pair const pair) {
  printf("\nTEST:  %s ", pair.key.begin);
  (void)p_state;
  return AZ_OK;
}

az_result az_curl_http_request(az_curl * const p_curl, az_http_request const * const p_request) {
  az_headers_data headers = {
    .p_list = NULL,
  };
  (void)p_curl;

  az_pair_visitor const pair_visitor = az_create_headers_callback(&headers, az_headers_to_curl);
  az_pair_seq const h = p_request->headers;
  AZ_RETURN_IF_FAILED(h.func(h.data, pair_visitor));

  return AZ_OK;
}
