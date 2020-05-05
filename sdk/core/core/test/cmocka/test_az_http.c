// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_json_string_private.h"
#include "az_test_definitions.h"
#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_private.h>
#include <az_http_transport.h>
#include <az_json.h>
#include <az_span.h>

#include <az_precondition.h>
#include <az_precondition_internal.h>

#include <setjmp.h>
#include <stdarg.h>

#include <az_test_precondition.h>
#include <cmocka.h>

#include <_az_cfg.h>

#define TEST_EXPECT_SUCCESS(exp) assert_true(az_succeeded(exp))

static az_span hrb_url
    = AZ_SPAN_LITERAL_FROM_STR("https://antk-keyvault.vault.azure.net/secrets/Password");

static az_span hrb_param_api_version_name = AZ_SPAN_LITERAL_FROM_STR("api-version");
static az_span hrb_param_api_version_token = AZ_SPAN_LITERAL_FROM_STR("7.0");

static az_span hrb_url2 = AZ_SPAN_LITERAL_FROM_STR(
    "https://antk-keyvault.vault.azure.net/secrets/Password?api-version=7.0");

static az_span hrb_param_test_param_name = AZ_SPAN_LITERAL_FROM_STR("test-param");
static az_span hrb_param_test_param_token = AZ_SPAN_LITERAL_FROM_STR("token");

static az_span hrb_url3 = AZ_SPAN_LITERAL_FROM_STR(
    "https://antk-keyvault.vault.azure.net/secrets/Password?api-version=7.0&test-param=token");

static az_span hrb_header_content_type_name = AZ_SPAN_LITERAL_FROM_STR("Content-Type");
static az_span hrb_header_content_type_token
    = AZ_SPAN_LITERAL_FROM_STR("application/x-www-form-urlencoded");

static az_span hrb_header_authorization_name = AZ_SPAN_LITERAL_FROM_STR("authorization");
static az_span hrb_header_authorization_token1 = AZ_SPAN_LITERAL_FROM_STR("Bearer 123456789");
static az_span hrb_header_authorization_token2
    = AZ_SPAN_LITERAL_FROM_STR("Bearer 99887766554433221100");

static void test_http_request(void** state)
{
  (void)state;
  {
    uint8_t buf[100];
    uint8_t header_buf[(2 * sizeof(az_pair))];
    memset(buf, 0, sizeof(buf));
    memset(header_buf, 0, sizeof(header_buf));

    az_span url_span = AZ_SPAN_FROM_BUFFER(buf);
    az_span remainder = az_span_copy(url_span, hrb_url);
    assert_int_equal(az_span_size(remainder), 100 - az_span_size(hrb_url));
    az_span header_span = AZ_SPAN_FROM_BUFFER(header_buf);
    _az_http_request hrb;

    TEST_EXPECT_SUCCESS(az_http_request_init(
        &hrb,
        &az_context_app,
        az_http_method_get(),
        url_span,
        az_span_size(hrb_url),
        header_span,
        AZ_SPAN_FROM_STR("body")));
    assert_true(az_span_is_content_equal(hrb._internal.method, az_http_method_get()));
    assert_true(az_span_is_content_equal(hrb._internal.url, url_span));
    assert_int_equal(az_span_size(hrb._internal.url), 100);
    assert_int_equal(hrb._internal.url_length, 54);
    assert_true(hrb._internal.max_headers == 2);
    assert_true(hrb._internal.retry_headers_start_byte_offset == 0);

    TEST_EXPECT_SUCCESS(az_http_request_set_query_parameter(
        &hrb, hrb_param_api_version_name, hrb_param_api_version_token));
    assert_int_equal(hrb._internal.url_length, az_span_size(hrb_url2));
    assert_true(az_span_is_content_equal(
        az_span_slice(hrb._internal.url, 0, hrb._internal.url_length), hrb_url2));

    TEST_EXPECT_SUCCESS(az_http_request_set_query_parameter(
        &hrb, hrb_param_test_param_name, hrb_param_test_param_token));
    assert_int_equal(hrb._internal.url_length, az_span_size(hrb_url3));
    assert_true(az_span_is_content_equal(
        az_span_slice(hrb._internal.url, 0, hrb._internal.url_length), hrb_url3));

    TEST_EXPECT_SUCCESS(az_http_request_append_header(
        &hrb, hrb_header_content_type_name, hrb_header_content_type_token));

    assert_true(hrb._internal.retry_headers_start_byte_offset == 0);

    TEST_EXPECT_SUCCESS(_az_http_request_mark_retry_headers_start(&hrb));
    assert_int_equal(hrb._internal.retry_headers_start_byte_offset, sizeof(az_pair));

    TEST_EXPECT_SUCCESS(az_http_request_append_header(
        &hrb, hrb_header_authorization_name, hrb_header_authorization_token1));

    assert_true(az_span_size(hrb._internal.headers) / (int32_t)sizeof(az_pair) == 2);
    assert_true(hrb._internal.retry_headers_start_byte_offset == sizeof(az_pair));

    az_pair expected_headers1[2] = {
      { .key = hrb_header_content_type_name, .value = hrb_header_content_type_token },
      { .key = hrb_header_authorization_name, .value = hrb_header_authorization_token1 },
    };
    for (uint16_t i = 0; i < az_span_size(hrb._internal.headers) / (int32_t)sizeof(az_pair); ++i)
    {
      az_pair header = { 0 };
      TEST_EXPECT_SUCCESS(az_http_request_get_header(&hrb, i, &header));

      assert_true(az_span_is_content_equal(header.key, expected_headers1[i].key));
      assert_true(az_span_is_content_equal(header.value, expected_headers1[i].value));
    }

    TEST_EXPECT_SUCCESS(_az_http_request_remove_retry_headers(&hrb));
    assert_true(hrb._internal.retry_headers_start_byte_offset == sizeof(az_pair));

    TEST_EXPECT_SUCCESS(az_http_request_append_header(
        &hrb, hrb_header_authorization_name, hrb_header_authorization_token2));
    assert_int_equal(hrb._internal.headers_length, 2);
    assert_int_equal(az_span_size(hrb._internal.headers) / (int32_t)sizeof(az_pair), 2);
    assert_true(hrb._internal.retry_headers_start_byte_offset == sizeof(az_pair));

    az_pair expected_headers2[2] = {
      { .key = hrb_header_content_type_name, .value = hrb_header_content_type_token },
      { .key = hrb_header_authorization_name, .value = hrb_header_authorization_token2 },
    };
    for (uint16_t i = 0; i < az_span_size(hrb._internal.headers) / (int32_t)sizeof(az_pair); ++i)
    {
      az_pair header = { 0 };
      TEST_EXPECT_SUCCESS(az_http_request_get_header(&hrb, i, &header));

      assert_true(az_span_is_content_equal(header.key, expected_headers2[i].key));
      assert_true(az_span_is_content_equal(header.value, expected_headers2[i].value));
    }

    assert_return_code(az_http_request_append_path(&hrb, AZ_SPAN_FROM_STR("path")), AZ_OK);

    az_http_method method;
    az_span body;
    az_span url;
    assert_return_code(az_http_request_get_method(&hrb, &method), AZ_OK);
    assert_return_code(az_http_request_get_url(&hrb, &url), AZ_OK);
    assert_return_code(az_http_request_get_body(&hrb, &body), AZ_OK);
    assert_string_equal(az_span_ptr(method), az_span_ptr(az_http_method_get()));
    assert_string_equal(az_span_ptr(body), az_span_ptr(AZ_SPAN_FROM_STR("body")));
    assert_string_equal(
        az_span_ptr(url),
        az_span_ptr(AZ_SPAN_FROM_STR("https://antk-keyvault.vault.azure.net/secrets/Password/"
                                     "path?api-version=7.0&test-param=token")));
  }
  {
    uint8_t buf[100];
    uint8_t header_buf[(2 * sizeof(az_pair))];
    memset(buf, 0, sizeof(buf));
    memset(header_buf, 0, sizeof(header_buf));

    az_span url_span = AZ_SPAN_FROM_BUFFER(buf);
    az_span remainder = az_span_copy(url_span, hrb_url);
    assert_int_equal(az_span_size(remainder), 100 - az_span_size(hrb_url));
    az_span header_span = AZ_SPAN_FROM_BUFFER(header_buf);
    _az_http_request hrb;

    TEST_EXPECT_SUCCESS(az_http_request_init(
        &hrb,
        &az_context_app,
        az_http_method_get(),
        url_span,
        az_span_size(hrb_url),
        header_span,
        AZ_SPAN_FROM_STR("body")));

    // Empty header
    assert_return_code(
        az_http_request_append_header(&hrb, AZ_SPAN_FROM_STR("header"), AZ_SPAN_NULL), AZ_OK);
  }
  {
    uint8_t buf[100];
    uint8_t header_buf[(2 * sizeof(az_pair))];
    memset(buf, 0, sizeof(buf));
    memset(header_buf, 0, sizeof(header_buf));

    az_span url_span = AZ_SPAN_FROM_BUFFER(buf);
    az_span remainder = az_span_copy(url_span, hrb_url);
    assert_int_equal(az_span_size(remainder), 100 - az_span_size(hrb_url));
    az_span header_span = AZ_SPAN_FROM_BUFFER(header_buf);
    _az_http_request hrb;

    TEST_EXPECT_SUCCESS(az_http_request_init(
        &hrb,
        &az_context_app,
        az_http_method_get(),
        url_span,
        az_span_size(hrb_url),
        header_span,
        AZ_SPAN_FROM_STR("body")));

    // Remove empty spaces from the left of header name
    assert_return_code(
        az_http_request_append_header(&hrb, AZ_SPAN_FROM_STR(" \t\r\nheader"), AZ_SPAN_NULL),
        AZ_OK);

    az_pair header = { 0 };
    assert_return_code(az_http_request_get_header(&hrb, 0, &header), AZ_OK);
    assert_true(az_span_is_content_equal(header.key, AZ_SPAN_FROM_STR("header")));
  }
}

static void test_http_request_header_validation_above_127(void** state)
{
  (void)state;
  {
    uint8_t header_buf[(2 * sizeof(az_pair))];
    memset(header_buf, 0, sizeof(header_buf));

    az_span url_span = AZ_SPAN_FROM_STR("some.url.com");
    az_span header_span = AZ_SPAN_FROM_BUFFER(header_buf);
    _az_http_request hrb;

    TEST_EXPECT_SUCCESS(az_http_request_init(
        &hrb,
        &az_context_app,
        az_http_method_get(),
        url_span,
        az_span_size(url_span),
        header_span,
        AZ_SPAN_FROM_STR("body")));

    uint8_t c[1] = { 255 };
    az_span header_name = AZ_SPAN_FROM_BUFFER(c);
    az_result r = az_http_request_append_header(&hrb, header_name, hrb_header_content_type_token);

    assert_true(r = AZ_ERROR_ARG);
  }
}

#define EXAMPLE_BODY \
  "{\r\n" \
  "  \"somejson\":45\r" \
  "}\n"

static void test_http_response(void** state)
{
  (void)state;
  // no headers
  {
    az_span response_span = AZ_SPAN_FROM_STR( //
        "HTTP/1.2 404 We removed the\tpage!\r\n"
        "\r\n"
        "But there is somebody. :-)");
    az_http_response response = { 0 };
    az_result result = az_http_response_init(&response, response_span);
    assert_true(result == AZ_OK);

    {
      az_http_response_status_line status_line = { 0 };
      result = az_http_response_get_status_line(&response, &status_line);
      assert_true(result == AZ_OK);
      assert_true(status_line.major_version == 1);
      assert_true(status_line.minor_version == 2);
      assert_true(status_line.status_code == AZ_HTTP_STATUS_CODE_NOT_FOUND);
      assert_true(az_span_is_content_equal(
          status_line.reason_phrase, AZ_SPAN_FROM_STR("We removed the\tpage!")));
    }
    // try to read a header
    {
      az_pair header = { 0 };
      result = az_http_response_get_next_header(&response, &header);
      assert_true(result == AZ_ERROR_ITEM_NOT_FOUND);
    }
    // read a body
    {
      az_span body = { 0 };
      result = az_http_response_get_body(&response, &body);
      assert_true(result == AZ_OK);
      assert_true(az_span_is_content_equal(body, AZ_SPAN_FROM_STR("But there is somebody. :-)")));
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
    assert_true(result == AZ_OK);

    // read a status line
    {
      az_http_response_status_line status_line = { 0 };
      result = az_http_response_get_status_line(&response, &status_line);
      assert_true(result == AZ_OK);
      assert_true(status_line.major_version == 2);
      assert_true(status_line.minor_version == 0);
      assert_true(status_line.status_code == AZ_HTTP_STATUS_CODE_RESET_CONTENT);
      assert_true(az_span_is_content_equal(status_line.reason_phrase, AZ_SPAN_FROM_STR("")));
    }
    // read a header1
    {
      az_pair header = { 0 };
      result = az_http_response_get_next_header(&response, &header);
      assert_true(result == AZ_OK);
      assert_true(az_span_is_content_equal(header.key, AZ_SPAN_FROM_STR("header1")));
      assert_true(az_span_is_content_equal(header.value, AZ_SPAN_FROM_STR("some value")));
    }
    // read a Header2
    {
      az_pair header = { 0 };
      result = az_http_response_get_next_header(&response, &header);
      assert_true(result == AZ_OK);
      assert_true(az_span_is_content_equal(header.key, AZ_SPAN_FROM_STR("Header2")));
      assert_true(az_span_is_content_equal(header.value, AZ_SPAN_FROM_STR("something")));
    }
    // try to read a header
    {
      az_pair header = { 0 };
      result = az_http_response_get_next_header(&response, &header);
      assert_true(result == AZ_ERROR_ITEM_NOT_FOUND);
    }
    // read a body
    {
      az_span body = { 0 };
      result = az_http_response_get_body(&response, &body);
      assert_true(result == AZ_OK);
      assert_true(az_span_is_content_equal(body, AZ_SPAN_FROM_STR("")));
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
  assert_true(result == AZ_OK);

  // add parsing here
  {
    az_span body;
    TEST_EXPECT_SUCCESS(az_http_response_get_body(&response, &body));
    assert_true(az_span_is_content_equal(body, AZ_SPAN_FROM_STR(EXAMPLE_BODY)));
  }
}

#ifndef AZ_NO_PRECONDITION_CHECKING
enable_precondition_check_tests()

    static void test_http_request_removing_left_white_spaces(void** state)
{
  (void)state;

  uint8_t buf[100];
  uint8_t header_buf[(2 * sizeof(az_pair))];
  memset(buf, 0, sizeof(buf));
  memset(header_buf, 0, sizeof(header_buf));

  az_span url_span = AZ_SPAN_FROM_BUFFER(buf);
  az_span remainder = az_span_copy(url_span, hrb_url);
  assert_int_equal(az_span_size(remainder), 100 - az_span_size(hrb_url));
  az_span header_span = AZ_SPAN_FROM_BUFFER(header_buf);
  _az_http_request hrb;

  TEST_EXPECT_SUCCESS(az_http_request_init(
      &hrb,
      &az_context_app,
      az_http_method_get(),
      url_span,
      az_span_size(hrb_url),
      header_span,
      AZ_SPAN_FROM_STR("body")));

  // Nothing but empty name - should hit precondion
  assert_precondition_checked(
      az_http_request_append_header(&hrb, AZ_SPAN_FROM_STR(" \t\r"), AZ_SPAN_NULL));
}

static void test_http_request_header_validation(void** state)
{
  (void)state;
  {
    uint8_t header_buf[(2 * sizeof(az_pair))];
    memset(header_buf, 0, sizeof(header_buf));

    az_span url_span = AZ_SPAN_FROM_STR("some.url.com");
    az_span header_span = AZ_SPAN_FROM_BUFFER(header_buf);
    _az_http_request hrb;

    TEST_EXPECT_SUCCESS(az_http_request_init(
        &hrb,
        &az_context_app,
        az_http_method_get(),
        url_span,
        az_span_size(url_span),
        header_span,
        AZ_SPAN_FROM_STR("body")));

    az_result r = az_http_request_append_header(
        &hrb, AZ_SPAN_FROM_STR("(headerName)"), hrb_header_content_type_token);

    assert_true(r = AZ_ERROR_ARG);
  }
}

#endif // AZ_NO_PRECONDITION_CHECKING

int test_az_http()
{
#ifndef AZ_NO_PRECONDITION_CHECKING
  setup_precondition_check_tests();
#endif // AZ_NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_http_request_removing_left_white_spaces),
    cmocka_unit_test(test_http_request_header_validation),
#endif // AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_http_request),
    cmocka_unit_test(test_http_response),
    cmocka_unit_test(test_http_request_header_validation_above_127),
  };
  return cmocka_run_group_tests_name("az_core_http", tests, NULL, NULL);
}
