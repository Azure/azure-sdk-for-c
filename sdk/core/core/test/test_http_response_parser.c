// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_response_parser.h>
#include <az_span.h>
#include <az_str.h>

#include "./az_test.h"

#include <_az_cfg.h>

#define EXAMPLE_BODY \
  "{\r\n" \
  "  \"somejson\":45\r" \
  "}\n"

az_result http_response_parser_example(az_span const response) {
  // create a parser.
  az_http_response_parser parser;
  AZ_RETURN_IF_FAILED(az_http_response_parser_init(&parser, response));

  // make sure it's a known protocol "1.1" and a status code is Ok (200).
  {
    az_http_response_status_line status_line;
    AZ_RETURN_IF_FAILED(az_http_response_parser_read_status_line(&parser, &status_line));
    AZ_RETURN_IF_FAILED(status_line.major_version == 1);
    AZ_RETURN_IF_FAILED(status_line.minor_version == 1);
    AZ_RETURN_IF_FAILED(status_line.status_code == AZ_HTTP_STATUS_CODE_OK);
  }

  // reading headers
  {
    az_span contentType = { 0 };
    while (true) {
      az_pair header;
      // read a header
      {
        az_result const result = az_http_response_parser_read_header(&parser, &header);
        if (result == AZ_ERROR_ITEM_NOT_FOUND) {
          break;
        }
        AZ_RETURN_IF_FAILED(result);
      }
      // we expect only one header for now
      if (az_span_is_equal(header.key, AZ_STR("Content-Type"))) {
        contentType = header.value;
      } else {
        return AZ_ERROR_HTTP_INVALID_STATE;
      }
    }

    // make sure we have a Content-Type header and it's what we expect.
    if (!az_span_is_equal(contentType, AZ_STR("text/html; charset=UTF-8"))) {
      return AZ_ERROR_HTTP_INVALID_STATE;
    }
  }

  // reading body
  {
    az_span body;
    AZ_RETURN_IF_FAILED(az_http_response_parser_read_body(&parser, &body));
    if (!az_span_is_equal(body, AZ_STR(EXAMPLE_BODY))) {
      return AZ_ERROR_HTTP_INVALID_STATE;
    }
  }

  return AZ_OK;
}

static az_result http_response_getters_example(az_span const response) {

  // make sure it's a known protocol "1.1" and a status code is Ok (200).
  {
    az_http_response_status_line status_line;
    AZ_RETURN_IF_FAILED(az_http_response_get_status_line(response, &status_line));
    AZ_RETURN_IF_FAILED(status_line.major_version == 1);
    AZ_RETURN_IF_FAILED(status_line.minor_version == 1);
    AZ_RETURN_IF_FAILED(status_line.status_code == AZ_HTTP_STATUS_CODE_OK);
  }

  // Last header is used for reading body and must be properly initialized (set to zeros).
  az_pair header = { 0 };

  // reading headers
  {
    az_span contentType = { 0 };
    while (true) {
      // read a header
      {
        az_result const result = az_http_response_get_next_header(response, &header);
        if (result == AZ_ERROR_ITEM_NOT_FOUND) {
          break;
        }
        AZ_RETURN_IF_FAILED(result);
      }
      // we expect only one header for now
      if (az_span_is_equal(header.key, AZ_STR("Content-Type"))) {
        contentType = header.value;
      } else {
        return AZ_ERROR_HTTP_INVALID_STATE;
      }
    }

    // make sure we have a Content-Type header and it's what we expect.
    if (!az_span_is_equal(contentType, AZ_STR("text/html; charset=UTF-8"))) {
      return AZ_ERROR_HTTP_INVALID_STATE;
    }
  }

  // reading body
  {
    az_span body = { 0 };
    AZ_RETURN_IF_FAILED(az_http_response_get_body(response, &header, &body));
    if (!az_span_is_equal(body, AZ_STR(EXAMPLE_BODY))) {
      return AZ_ERROR_HTTP_INVALID_STATE;
    }
  }

  return AZ_OK;
}

void test_http_response_parser() {
  // no headers
  {
    az_span const response = AZ_STR( //
        "HTTP/1.2 404 We removed the\tpage!\r\n"
        "\r\n"
        "But there is somebody. :-)");
    az_http_response_parser parser;
    {
      az_result const result = az_http_response_parser_init(&parser, response);
      TEST_ASSERT(result == AZ_OK);
    }
    // read a status line
    {
      az_http_response_status_line status_line = { 0 };
      az_result const result = az_http_response_parser_read_status_line(&parser, &status_line);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(status_line.major_version == 1);
      TEST_ASSERT(status_line.minor_version == 2);
      TEST_ASSERT(status_line.status_code == 404);
      TEST_ASSERT(az_span_is_equal(status_line.reason_phrase, AZ_STR("We removed the\tpage!")));
    }
    // try to read a header
    {
      az_pair header = { 0 };
      az_result const result = az_http_response_parser_read_header(&parser, &header);
      TEST_ASSERT(result == AZ_ERROR_ITEM_NOT_FOUND);
    }
    // read a body
    {
      az_span body = { 0 };
      az_result const result = az_http_response_parser_read_body(&parser, &body);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(az_span_is_equal(body, AZ_STR("But there is somebody. :-)")));
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
      az_result const result = az_http_response_parser_init(&parser, response);
      TEST_ASSERT(result == AZ_OK);
    }
    // read a status line
    {
      az_http_response_status_line status_line = { 0 };
      az_result const result = az_http_response_parser_read_status_line(&parser, &status_line);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(status_line.major_version == 2);
      TEST_ASSERT(status_line.minor_version == 0);
      TEST_ASSERT(status_line.status_code == 205);
      TEST_ASSERT(az_span_is_equal(status_line.reason_phrase, AZ_STR("")));
    }
    // read a header1
    {
      az_pair header = { 0 };
      az_result const result = az_http_response_parser_read_header(&parser, &header);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(az_span_is_equal(header.key, AZ_STR("header1")));
      TEST_ASSERT(az_span_is_equal(header.value, AZ_STR("some value")));
    }
    // read a Header2
    {
      az_pair header = { 0 };
      az_result const result = az_http_response_parser_read_header(&parser, &header);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(az_span_is_equal(header.key, AZ_STR("Header2")));
      TEST_ASSERT(az_span_is_equal(header.value, AZ_STR("something")));
    }
    // try to read a header
    {
      az_pair header = { 0 };
      az_result const result = az_http_response_parser_read_header(&parser, &header);
      TEST_ASSERT(result == AZ_ERROR_ITEM_NOT_FOUND);
    }
    // read a body
    {
      az_span body = { 0 };
      az_result const result = az_http_response_parser_read_body(&parser, &body);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(az_span_is_equal(body, AZ_STR("")));
    }
  }

  az_span const response = AZ_STR( //
      "HTTP/1.1 200 Ok\r\n"
      "Content-Type: text/html; charset=UTF-8\r\n"
      "\r\n"
      // body
      EXAMPLE_BODY);

  // an example
  {
    az_result const result = http_response_parser_example(response);
    TEST_ASSERT(result == AZ_OK);
  }
  // an example of getters
  {
    az_result const result = http_response_getters_example(response);
    TEST_ASSERT(result == AZ_OK);
  }

  // get a header by name
  {
    az_span value;
    az_result const result
        = az_http_response_get_header_by_name(response, AZ_STR("Content-Type"), &value);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(az_span_is_equal(value, AZ_STR("text/html; charset=UTF-8")));
  }
  {
    az_span value;
    az_result const result
        = az_http_response_get_header_by_name(response, AZ_STR("conTent-typE"), &value);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(az_span_is_equal(value, AZ_STR("text/html; charset=UTF-8")));
  }
  {
    az_span value;
    az_result const result
        = az_http_response_get_header_by_name(response, AZ_STR("conTent-typA"), &value);
    TEST_ASSERT(result == AZ_ERROR_ITEM_NOT_FOUND);
  }
}
