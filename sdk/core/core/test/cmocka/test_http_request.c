// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_json_string_private.h"
#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_private.h>
#include <az_http_transport.h>
#include <az_json.h>
#include <az_span.h>

#include <setjmp.h>
#include <stdarg.h>

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

void test_http_request(void** state)
{
  (void)state;
  {
    uint8_t buf[100];
    uint8_t header_buf[(2 * sizeof(az_pair))];
    memset(buf, 0, sizeof(buf));
    memset(header_buf, 0, sizeof(header_buf));

    az_span url_span = AZ_SPAN_FROM_BUFFER(buf);
    TEST_EXPECT_SUCCESS(az_span_append(url_span, hrb_url, &url_span));
    az_span header_span = AZ_SPAN_FROM_BUFFER(header_buf);
    _az_http_request hrb;

    TEST_EXPECT_SUCCESS(az_http_request_init(
        &hrb, &az_context_app, az_http_method_get(), url_span, header_span, AZ_SPAN_NULL));
    assert_true(az_span_is_equal(hrb._internal.method, az_http_method_get()));
    assert_true(az_span_is_equal(hrb._internal.url, url_span));
    assert_true(az_span_capacity(hrb._internal.url) == 100);
    assert_true(hrb._internal.max_headers == 2);
    assert_true(hrb._internal.retry_headers_start_byte_offset == 0);

    TEST_EXPECT_SUCCESS(az_http_request_set_query_parameter(
        &hrb, hrb_param_api_version_name, hrb_param_api_version_token));
    assert_true(az_span_is_equal(hrb._internal.url, hrb_url2));

    TEST_EXPECT_SUCCESS(az_http_request_set_query_parameter(
        &hrb, hrb_param_test_param_name, hrb_param_test_param_token));
    assert_true(az_span_is_equal(hrb._internal.url, hrb_url3));

    TEST_EXPECT_SUCCESS(az_http_request_append_header(
        &hrb, hrb_header_content_type_name, hrb_header_content_type_token));

    assert_true(hrb._internal.retry_headers_start_byte_offset == 0);

    TEST_EXPECT_SUCCESS(_az_http_request_mark_retry_headers_start(&hrb));
    assert_true(hrb._internal.retry_headers_start_byte_offset == sizeof(az_pair));

    TEST_EXPECT_SUCCESS(az_http_request_append_header(
        &hrb, hrb_header_authorization_name, hrb_header_authorization_token1));

    assert_true(az_span_length(hrb._internal.headers) / sizeof(az_pair) == 2);
    assert_true(hrb._internal.retry_headers_start_byte_offset == sizeof(az_pair));

    az_pair expected_headers1[2] = {
      { .key = hrb_header_content_type_name, .value = hrb_header_content_type_token },
      { .key = hrb_header_authorization_name, .value = hrb_header_authorization_token1 },
    };
    for (uint16_t i = 0; i < az_span_length(hrb._internal.headers) / sizeof(az_pair); ++i)
    {
      az_pair header = { 0 };
      TEST_EXPECT_SUCCESS(az_http_request_get_header(&hrb, i, &header));

      assert_true(az_span_is_equal(header.key, expected_headers1[i].key));
      assert_true(az_span_is_equal(header.value, expected_headers1[i].value));
    }

    TEST_EXPECT_SUCCESS(_az_http_request_remove_retry_headers(&hrb));
    assert_true(hrb._internal.retry_headers_start_byte_offset == sizeof(az_pair));

    TEST_EXPECT_SUCCESS(az_http_request_append_header(
        &hrb, hrb_header_authorization_name, hrb_header_authorization_token2));
    assert_true(az_span_length(hrb._internal.headers) / sizeof(az_pair) == 2);
    assert_true(hrb._internal.retry_headers_start_byte_offset == sizeof(az_pair));

    az_pair expected_headers2[2] = {
      { .key = hrb_header_content_type_name, .value = hrb_header_content_type_token },
      { .key = hrb_header_authorization_name, .value = hrb_header_authorization_token2 },
    };
    for (uint16_t i = 0; i < az_span_length(hrb._internal.headers) / sizeof(az_pair); ++i)
    {
      az_pair header = { 0 };
      TEST_EXPECT_SUCCESS(az_http_request_get_header(&hrb, i, &header));

      assert_true(az_span_is_equal(header.key, expected_headers2[i].key));
      assert_true(az_span_is_equal(header.value, expected_headers2[i].value));
    }
  }
}
