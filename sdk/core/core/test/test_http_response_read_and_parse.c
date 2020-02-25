// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http.h>
#include <az_span.h>

#include "./az_test.h"

#include <_az_cfg.h>

#define EXAMPLE_BODY \
  "{\r\n" \
  "  \"somejson\":45\r" \
  "}\n"

void test_http_response()
{
  // no headers
  {
    az_span response_span = AZ_SPAN_FROM_STR( //
        "HTTP/1.2 404 We removed the\tpage!\r\n"
        "\r\n"
        "But there is somebody. :-)");
    az_http_response response = { 0 };
    az_result result = az_http_response_init(&response, response_span);
    TEST_ASSERT(result == AZ_OK);

    {
      az_http_response_status_line status_line = { 0 };
      result = az_http_response_get_status_line(&response, &status_line);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(status_line.major_version == 1);
      TEST_ASSERT(status_line.minor_version == 2);
      TEST_ASSERT(status_line.status_code == AZ_HTTP_STATUS_CODE_NOT_FOUND);
      TEST_ASSERT(
          az_span_is_equal(status_line.reason_phrase, AZ_SPAN_FROM_STR("We removed the\tpage!")));
    }
    // try to read a header
    {
      az_pair header = { 0 };
      result = az_http_response_get_next_header(&response, &header);
      TEST_ASSERT(result == AZ_ERROR_ITEM_NOT_FOUND);
    }
    // read a body
    {
      az_span body = { 0 };
      result = az_http_response_get_body(&response, &body);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(az_span_is_equal(body, AZ_SPAN_FROM_STR("But there is somebody. :-)")));
    }
  }

  // headers, no reason and no body.
  {
    az_span response_span = AZ_SPAN_FROM_STR( //
        "HTTP/2.0 205 \r\n"
        "header1: some value\r\n"
        "Header2:something\t   \r\n"
        "\r\n");

    az_http_response response = { 0 };
    az_result result = az_http_response_init(&response, response_span);
    TEST_ASSERT(result == AZ_OK);

    // read a status line
    {
      az_http_response_status_line status_line = { 0 };
      result = az_http_response_get_status_line(&response, &status_line);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(status_line.major_version == 2);
      TEST_ASSERT(status_line.minor_version == 0);
      TEST_ASSERT(status_line.status_code == AZ_HTTP_STATUS_CODE_RESET_CONTENT);
      TEST_ASSERT(az_span_is_equal(status_line.reason_phrase, AZ_SPAN_FROM_STR("")));
    }
    // read a header1
    {
      az_pair header = { 0 };
      result = az_http_response_get_next_header(&response, &header);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(az_span_is_equal(header.key, AZ_SPAN_FROM_STR("header1")));
      TEST_ASSERT(az_span_is_equal(header.value, AZ_SPAN_FROM_STR("some value")));
    }
    // read a Header2
    {
      az_pair header = { 0 };
      result = az_http_response_get_next_header(&response, &header);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(az_span_is_equal(header.key, AZ_SPAN_FROM_STR("Header2")));
      TEST_ASSERT(az_span_is_equal(header.value, AZ_SPAN_FROM_STR("something")));
    }
    // try to read a header
    {
      az_pair header = { 0 };
      result = az_http_response_get_next_header(&response, &header);
      TEST_ASSERT(result == AZ_ERROR_ITEM_NOT_FOUND);
    }
    // read a body
    {
      az_span body = { 0 };
      result = az_http_response_get_body(&response, &body);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(az_span_is_equal(body, AZ_SPAN_FROM_STR("")));
    }
  }

  az_span response_span = AZ_SPAN_FROM_STR( //
      "HTTP/1.1 200 Ok\r\n"
      "Content-Type: text/html; charset=UTF-8\r\n"
      "\r\n"
      // body
      EXAMPLE_BODY);

  az_http_response response = { 0 };
  az_result const result = az_http_response_init(&response, response_span);
  TEST_ASSERT(result == AZ_OK);

  // add parsing here
  {
    az_span body;
    TEST_EXPECT_SUCCESS(az_http_response_get_body(&response, &body));
    TEST_ASSERT(az_span_is_equal(body, AZ_SPAN_FROM_STR(EXAMPLE_BODY)));
  }
}
