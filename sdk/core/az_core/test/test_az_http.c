// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_http_header_validation_private.h"
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
ENABLE_PRECONDITION_CHECK_TESTS()

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
  ASSERT_PRECONDITION_CHECKED(
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

    ASSERT_PRECONDITION_CHECKED(az_http_request_append_header(
        &hrb, AZ_SPAN_FROM_STR("(headerName)"), hrb_header_content_type_token));

    // make sure about header was not added
    assert_int_equal(az_http_request_headers_count(&hrb), 0);
    _az_http_request_headers headers = hrb._internal.headers;
    size_t size = (size_t)az_span_size(headers);
    assert_memory_equal(header_buf, headers._internal.ptr, size);
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
    ASSERT_PRECONDITION_CHECKED(
        az_http_request_append_header(&hrb, header_name, hrb_header_content_type_token));

    // make sure about header was not added
    assert_int_equal(az_http_request_headers_count(&hrb), 0);
    _az_http_request_headers headers = hrb._internal.headers;
    size_t size = (size_t)az_span_size(headers);
    assert_memory_equal(header_buf, headers._internal.ptr, size);
  }
}

static void test_http_response_append_null_response(void** state)
{
  (void)state;
  {
    az_http_response* ptr = NULL;
    ASSERT_PRECONDITION_CHECKED(az_http_response_append(ptr, AZ_SPAN_FROM_STR("test")));
  }
}

#endif // AZ_NO_PRECONDITION_CHECKING

static void test_http_request_header_validation_range(void** state)
{
  (void)state;
  {
    // just make sure compiler will set anything greater than 127 to zero
    for (int32_t value = 127; value < 256; value++)
    {
      assert_false(az_http_valid_token[(uint8_t)value]);
    }
  }
}

static void test_http_response_header_validation(void** state)
{
  (void)state;
  {
    az_http_response response = { 0 };
    assert_return_code(
        az_http_response_init(
            &response,
            AZ_SPAN_FROM_STR("HTTP/1.1 404 Not Found\r\n"
                             "Header11: Value11\r\n"
                             "Header22: NNNNOOOOPPPPQQQQRRRRSSSSTTTTUUUUVVVVWWWWXXXXYYYYZZZZ\r\n"
                             "Header33:\r\n"
                             "Header44: cba888888777777666666555555444444333333222222111111\r\n"
                             "\r\n"
                             "KKKKKJJJJJIIIIIHHHHHGGGGGFFFFFEEEEEDDDDDCCCCCBBBBBAAAAA")),
        AZ_OK);

    az_http_response_status_line status_line = { 0 };
    assert_return_code(az_http_response_get_status_line(&response, &status_line), AZ_OK);
    for (az_pair header;
         az_http_response_get_next_header(&response, &header) != AZ_ERROR_ITEM_NOT_FOUND;)
    {
      // all valid headers
      assert_true(az_span_ptr(header.key) != NULL);
      assert_true(az_span_size(header.key) > 0);
      assert_true(az_span_ptr(header.value) != NULL);
    }
  }
}

static void test_http_response_header_validation_fail(void** state)
{
  (void)state;
  {
    az_http_response response = { 0 };
    assert_return_code(
        az_http_response_init(
            &response,
            AZ_SPAN_FROM_STR("HTTP/1.1 404 Not Found\r\n"
                             "(Header11): Value11\r\n"
                             "\r\n"
                             "KKKKKJJJJJIIIIIHHHHHGGGGGFFFFFEEEEEDDDDDCCCCCBBBBBAAAAA")),
        AZ_OK);

    az_http_response_status_line status_line = { 0 };
    assert_return_code(az_http_response_get_status_line(&response, &status_line), AZ_OK);
    az_pair header = { 0 };
    az_result fail_header_result = az_http_response_get_next_header(&response, &header);
    assert_true(AZ_ERROR_HTTP_CORRUPT_RESPONSE_HEADER == fail_header_result);
  }
}

static void test_http_response_header_validation_space(void** state)
{
  (void)state;
  {
    az_http_response response = { 0 };
    assert_return_code(
        az_http_response_init(
            &response,
            AZ_SPAN_FROM_STR("HTTP/1.1 404 Not Found\r\n"
                             "   Header11     :         Value11\r\n"
                             "\r\n"
                             "KKKKKJJJJJIIIIIHHHHHGGGGGFFFFFEEEEEDDDDDCCCCCBBBBBAAAAA")),
        AZ_OK);

    az_http_response_status_line status_line = { 0 };
    assert_return_code(az_http_response_get_status_line(&response, &status_line), AZ_OK);
    az_pair header = { 0 };
    // Spaces in headers are Fine, we trim them
    assert_return_code(az_http_response_get_next_header(&response, &header), AZ_OK);
    assert_true(az_span_is_content_equal(header.key, AZ_SPAN_FROM_STR("Header11")));
    assert_true(az_span_is_content_equal(header.value, AZ_SPAN_FROM_STR("Value11")));
  }
}

static void test_http_response_append(void** state)
{
  (void)state;
  {
    uint8_t buffer[10];
    az_http_response response = { 0 };
    az_span append_this = AZ_SPAN_FROM_STR("0123456789");
    assert_return_code(az_http_response_init(&response, AZ_SPAN_FROM_BUFFER(buffer)), AZ_OK);
    assert_return_code(az_http_response_append(&response, append_this), AZ_OK);
    assert_memory_equal(buffer, az_span_ptr(append_this), 10);
  }
}

static void test_http_response_append_overflow(void** state)
{
  (void)state;
  {
    uint8_t buffer[] = "...........";
    az_http_response response = { 0 };
    az_span append_this = AZ_SPAN_FROM_STR("0123456789123456");

    assert_return_code(az_http_response_init(&response, AZ_SPAN_FROM_BUFFER(buffer)), AZ_OK);

    int32_t response_written_state = response._internal.written;

    az_result result = az_http_response_append(&response, append_this);
    assert_true(result == AZ_ERROR_INSUFFICIENT_SPAN_SIZE);

    // make sure buffer content didn't change
    assert_memory_equal(buffer, "..........", 10);

    // make sure response state didn't change
    assert_int_equal(response_written_state, response._internal.written);
  }
}

static void test_http_response_append_overflow_on_second_call(void** state)
{
  (void)state;
  {
    uint8_t buffer[] = "..........";
    az_http_response response = { 0 };
    az_span this_will_fit_once = AZ_SPAN_FROM_STR("0123456");
    assert_return_code(az_http_response_init(&response, AZ_SPAN_FROM_BUFFER(buffer)), AZ_OK);

    assert_return_code(az_http_response_append(&response, this_will_fit_once), AZ_OK);

    az_result result = az_http_response_append(&response, this_will_fit_once);
    assert_true(result == AZ_ERROR_INSUFFICIENT_SPAN_SIZE);
    assert_memory_equal(buffer, az_span_ptr(this_will_fit_once), 7);
    assert_memory_equal(buffer + 7, "...", 3);
  }
}

int test_az_http()
{
#ifndef AZ_NO_PRECONDITION_CHECKING
  SETUP_PRECONDITION_CHECK_TESTS();
#endif // AZ_NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_http_request_removing_left_white_spaces),
    cmocka_unit_test(test_http_request_header_validation),
    cmocka_unit_test(test_http_request_header_validation_above_127),
    cmocka_unit_test(test_http_response_append_null_response),
#endif // AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_http_request),
    cmocka_unit_test(test_http_response),
    cmocka_unit_test(test_http_request_header_validation_range),
    cmocka_unit_test(test_http_response_header_validation),
    cmocka_unit_test(test_http_response_header_validation_fail),
    cmocka_unit_test(test_http_response_header_validation_space),
    cmocka_unit_test(test_http_response_append_overflow),
    cmocka_unit_test(test_http_response_append),
    cmocka_unit_test(test_http_response_append_overflow_on_second_call),
  };
  return cmocka_run_group_tests_name("az_core_http", tests, NULL, NULL);
}
