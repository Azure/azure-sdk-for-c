// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_response_read.h>
#include <az_span.h>

#include "./az_test.h"

static void test_http_response_parser() {
  az_span const response = AZ_STR( //
      "HTTP/1.2 404 We removed the\tpage!\r\n"
      "\r\n"
      "But there is somebody. :-)");
  az_http_response_state state = az_http_response_state_create(response);
  // reade status line
  {
    az_http_response_value value = { 0 };
    az_result const result = az_http_response_state_read(&state, &value);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(value.kind == AZ_HTTP_RESPONSE_STATUS_LINE);
    az_http_response_status_line const status_line = value.data.status_line;
    TEST_ASSERT(status_line.major_version == 1);
    TEST_ASSERT(status_line.minor_version == 2);
    TEST_ASSERT(status_line.status_code == 404);
    TEST_ASSERT(az_span_eq(status_line.reason_phrase, AZ_STR("We removed the\tpage!")));
  }
  // reade body
  {
    az_http_response_value value = { 0 };
    az_result const result = az_http_response_state_read(&state, &value);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(value.kind == AZ_HTTP_RESPONSE_BODY);
    az_http_response_body const body = value.data.body;
    TEST_ASSERT(az_span_eq(body, AZ_STR("But there is somebody. :-)")));
  }
}
