// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_client.h>
#include <az_str.h>
#include <stdio.h>

#define TEST_URL_CONST "http://www.laboratoons.com/wp-content/uploads/2018/11/Alternativas-A.jpg"
#define TEST_FILE_CONST "downloadTest.jpg"

static az_const_span TEST_URL = AZ_CONST_STR(TEST_URL_CONST);
static az_const_span TEST_FILE = AZ_CONST_STR(TEST_FILE_CONST);

int main() {
  az_pair const query_array[] = {
    { .key = AZ_STR("hello"), .value = AZ_STR("world!") },
    { .key = AZ_STR("x"), .value = AZ_STR("42") },
  };

  az_pair const headers_array[] = {
    { .key = AZ_STR("some"), .value = AZ_STR("xml") },
    { .key = AZ_STR("xyz"), .value = AZ_STR("very_long") },
  };

  az_http_request const request = {
    .method = AZ_STR("GET"),
    .path = AZ_STR("/foo"),
    .query = az_pair_span_to_iter((az_pair_span)AZ_SPAN(query_array)),
    .headers = az_pair_span_to_iter((az_pair_span)AZ_SPAN(headers_array)),
    .body = AZ_STR("{ \"somejson\": true }"),
  };

  az_http_client_send(&request);

  return 0;
}
