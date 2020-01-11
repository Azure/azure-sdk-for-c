// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_response_parser.h>

#define EXAMPLE_BODY \
  "{\r\n" \
  "  \"somejson\":45\r" \
  "}\n"

static az_result http_response_parser_example(az_span const response) {

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
  az_pair header = { { 0 } };

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
