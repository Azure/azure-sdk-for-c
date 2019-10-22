// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdio.h>

#include <az_http_client.h>

int exit_code = 0;

int main() {
  az_pair const query_array[] = {
    { .key = AZ_STR("arg1"), .value = AZ_STR("curl_arg1") },
    { .key = AZ_STR("arg2"), .value = AZ_STR("curl_arg2") },
  };
  az_pair_span const query = AZ_SPAN(query_array);
  //
  az_pair const headers_array[] = {
    { .key = AZ_STR("header1"), .value = AZ_STR("h1") },
    { .key = AZ_STR("header2"), .value = AZ_STR("h2") },
  };
  az_pair_span const headers = AZ_SPAN(headers_array);
  //
  az_http_request const request = {
    .method = AZ_STR("GET"),
    .path = AZ_STR("http://127.0.0.1:5000/test/yo"),
    .query = az_pair_span_to_seq_callback(&query),
    .headers = az_pair_span_to_seq_callback(&headers),
    .body = AZ_STR("{ \"somejson\": true }"),
  };

  az_http_client_send_request(&request);
  return exit_code;
}
