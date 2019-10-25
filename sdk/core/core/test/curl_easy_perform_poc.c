// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_client.h>
#include <az_http_request_builder.h>
#include <az_json_read.h>
#include <az_pair.h>
#include <az_span_builder.h>
#include <stdlib.h>

#include <stdio.h>

#include <_az_cfg.h>

static az_const_span hrb_url = AZ_CONST_STR("http://127.0.0.1:5000/test/yo?arg1=vh");

static az_const_span hrb_header_content_type_name = AZ_CONST_STR("Content-Type");
static az_const_span hrb_header_content_type_value = AZ_CONST_STR("testThis");

int main() {
  // create a buffer for request
  uint8_t buf[1024 * 4];
  az_span const http_buf = AZ_SPAN(buf);
  az_http_request_builder hrb;

  // init buffer
  az_http_request_builder_init(&hrb, http_buf, AZ_HTTP_METHOD_VERB_GET, hrb_url, 100, 2);

  // add header
  az_http_request_builder_append_header(
      &hrb, hrb_header_content_type_name, hrb_header_content_type_value);

  az_http_client_send_request(&hrb, NULL);

  return 0;
}
