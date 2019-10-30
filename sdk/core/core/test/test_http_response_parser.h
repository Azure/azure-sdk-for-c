// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_response_parser.h>
#include <az_span.h>

#include "./az_test.h"

static void test_http_response_parser() {
  // no headers
  {
    az_span const response = AZ_STR( //
        "HTTP/1.2 404 We removed the\tpage!\r\n"
        "\r\n"
        "But there is somebody. :-)");
    az_http_response_parser parser;
    {
      az_result const result = az_http_response_parser_init(response, &parser);
      TEST_ASSERT(result == AZ_OK);
    }
    // read a status line
    {
      az_http_response_value value = { 0 };
      az_result const result = az_http_response_parser_read(&parser, &value);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(value.kind == AZ_HTTP_RESPONSE_STATUS_LINE);
      az_http_response_status_line const status_line = value.data.status_line;
      TEST_ASSERT(status_line.major_version == 1);
      TEST_ASSERT(status_line.minor_version == 2);
      TEST_ASSERT(status_line.status_code == 404);
      TEST_ASSERT(az_span_eq(status_line.reason_phrase, AZ_STR("We removed the\tpage!")));
    }
    // read a body
    {
      az_http_response_value value = { 0 };
      az_result const result = az_http_response_parser_read(&parser, &value);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(value.kind == AZ_HTTP_RESPONSE_BODY);
      az_http_response_body const body = value.data.body;
      TEST_ASSERT(az_span_eq(body, AZ_STR("But there is somebody. :-)")));
    }
  }
  // headers, no reason and no body.
  {
    az_span const response = AZ_STR( //
        "HTTP/2.0 205 \r\n"
        "header1: some value\r\n"
        "Header2:something\t   \r\n"
        "\r\n");
    az_http_response_parser parser;
    {
      az_result const result = az_http_response_parser_init(response, &parser);
      TEST_ASSERT(result == AZ_OK);
    }
    // read a status line
    {
      az_http_response_value value = { 0 };
      az_result const result = az_http_response_parser_read(&parser, &value);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(value.kind == AZ_HTTP_RESPONSE_STATUS_LINE);
      az_http_response_status_line const status_line = value.data.status_line;
      TEST_ASSERT(status_line.major_version == 2);
      TEST_ASSERT(status_line.minor_version == 0);
      TEST_ASSERT(status_line.status_code == 205);
      TEST_ASSERT(az_span_eq(status_line.reason_phrase, AZ_STR("")));
    }
    // read a header1
    {
      az_http_response_value value = { 0 };
      az_result const result = az_http_response_parser_read(&parser, &value);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(value.kind == AZ_HTTP_RESPONSE_HEADER);
      az_http_response_header const header = value.data.header;
      TEST_ASSERT(az_span_eq(header.key, AZ_STR("header1")));
      TEST_ASSERT(az_span_eq(header.value, AZ_STR("some value")));
    }
    // read a Header2
    {
      az_http_response_value value = { 0 };
      az_result const result = az_http_response_parser_read(&parser, &value);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(value.kind == AZ_HTTP_RESPONSE_HEADER);
      az_http_response_header const header = value.data.header;
      TEST_ASSERT(az_span_eq(header.key, AZ_STR("Header2")));
      TEST_ASSERT(az_span_eq(header.value, AZ_STR("something")));
    }
    // read a body
    {
      az_http_response_value value = { 0 };
      az_result const result = az_http_response_parser_read(&parser, &value);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(value.kind == AZ_HTTP_RESPONSE_BODY);
      az_http_response_body const body = value.data.body;
      TEST_ASSERT(az_span_eq(body, AZ_STR("")));
    }
  }
}
