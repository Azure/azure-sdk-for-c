// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdio.h>

#include <_az_cfg.h>
#include <az_curl.h>

int exit_code = 0;

int main() {
  az_pair const query_array[] = {
    { .key = AZ_STR("hello"), .value = AZ_STR("world!") },
    { .key = AZ_STR("x"), .value = AZ_STR("42") },
  };
  az_pair_span const query = AZ_SPAN(query_array);
  //
  az_pair const headers_array[] = {
    { .key = AZ_STR("some"), .value = AZ_STR("xml") },
    { .key = AZ_STR("xyz"), .value = AZ_STR("very_long") },
  };
  az_pair_span const headers = AZ_SPAN(headers_array);
  //
  az_http_request const request = {
    .method = AZ_STR("GET"),
    .path = AZ_STR("/foo"),
    .query = az_pair_span_to_seq(&query),
    .headers = az_pair_span_to_seq(&headers),
    .body = AZ_STR("{ \"somejson\": true }"),
  };

  az_curl p_c;
  az_curl_init(&p_c);
  az_curl_http_request(&p_c, &request);
  az_curl_done(&p_c);

  return exit_code;
}
