// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <az_aad_private.h>
#include <az_credentials.h>
#include <az_span.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <_az_cfg.h>

/* Mocked function from PAL
 *  Using link option -ld wrap, we replace az_platform_clock_msec with
 *  the one implemented here where we can mock the return value of it
 */
int64_t __wrap_az_platform_clock_msec();

az_result __wrap_az_http_client_send_request(
    _az_http_request* p_request,
    az_http_response* p_response);

int64_t __wrap_az_platform_clock_msec() { return (int64_t)mock(); }

az_span http_response_span = AZ_SPAN_LITERAL_FROM_STR("HTTP/1.1 200 Ok\r\n"
                                                      "Content-Type: text/html; charset=UTF-8\r\n"
                                                      "\r\n"
                                                      "{\r\n"
                                                      "  \"expires_in\":500,\r"
                                                      "  \"access_token\":\"fakeToken\"\r"
                                                      "}\n");
az_result __wrap_az_http_client_send_request(
    _az_http_request* p_request,
    az_http_response* p_response)
{
  // Mocked transport that sets p_response to a valid HTTP token response for aad
  (void)p_request;
  AZ_RETURN_IF_FAILED(az_http_response_init(p_response, http_response_span));
  return AZ_OK;
}

static void test_az_token_expired()
{
  _az_token token = { 0 };
  token._internal.expires_at_msec = 100;

  will_return(__wrap_az_platform_clock_msec, 200);
  assert_true(_az_token_expired(&token));

  will_return(__wrap_az_platform_clock_msec, 0);
  assert_false(_az_token_expired(&token));

  token._internal.expires_at_msec = 0;
  assert_true(_az_token_expired(&token));
}

static void test_az_aad_request_token()
{
  // Calling az_aad_request_token(_az_http_request* ref_request, _az_token* out_token);
  _az_token t = { 0 };
  _az_http_request r = { 0 };
  will_return(__wrap_az_platform_clock_msec, 0);
  assert_return_code(_az_aad_request_token(&r, &t), AZ_OK);
  assert_string_equal(t._internal.token, "Bearer fakeToken");
}

static void test_az_aad_build_body()
{
  uint8_t buffer[500];
  az_span body = AZ_SPAN_FROM_BUFFER(buffer);
  assert_return_code(
      _az_aad_build_body(
          body,
          AZ_SPAN_FROM_STR("client_id"),
          AZ_SPAN_FROM_STR("scopes"),
          AZ_SPAN_FROM_STR("secret"),
          &body),
      AZ_OK);
}

static void test_az_aad_build_url()
{
  uint8_t buffer[500];
  az_span url = AZ_SPAN_FROM_BUFFER(buffer);
  assert_return_code(_az_aad_build_url(url, AZ_SPAN_FROM_STR("tenant"), &url), AZ_OK);
}

void test_az_aad(void** state)
{
  (void)state;

/* Tests using wrap to mock. Only suported by gcc */
#ifdef MOCK_ENABLED
  test_az_token_expired();
  test_az_aad_request_token();
#endif // MOCK_ENABLED

  test_az_aad_build_body();
  test_az_aad_build_url();
}
