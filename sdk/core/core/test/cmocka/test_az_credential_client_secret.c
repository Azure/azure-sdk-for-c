// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_credentials.h"
#include "az_test_definitions.h"
#include <az_credentials_internal.h>
#include <az_http.h>
#include <az_http_internal.h>
#include <az_span.h>

#include <stddef.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <_az_cfg.h>

static void test_credential_client_secret(void** state)
{
  (void)state;

  az_credential_client_secret credential = { 0 };
  // init credential_credentials struc
  assert_true(az_succeeded(az_credential_client_secret_init(
      &credential,
      AZ_SPAN_FROM_STR("TenantID"),
      AZ_SPAN_FROM_STR("ClientID"),
      AZ_SPAN_FROM_STR("ClientSecret"))));

  assert_true(az_succeeded(
      _az_credential_set_scopes((_az_credential*)&credential, AZ_SPAN_FROM_STR("Scopes"))));

  _az_http_pipeline pipeline = (_az_http_pipeline){
    ._internal = {
      .p_policies = {
        {._internal = { .process = az_http_pipeline_policy_credential, .p_options = &credential, }, },
        {._internal = { .process = az_http_pipeline_policy_transport, .p_options = NULL, }, },
      },
    },
  };

  az_result ignore = { 0 };

  az_span const request_url = AZ_SPAN_FROM_STR("https://www.microsoft.com/test/request");
  uint8_t header_buf[500];
  uint8_t body_buf[500];
  _az_http_request request = { 0 };
  ignore = az_http_request_init(
      &request,
      &az_context_app,
      az_http_method_get(),
      request_url,
      0,
      AZ_SPAN_FROM_BUFFER(header_buf),
      AZ_SPAN_FROM_BUFFER(body_buf));

  az_http_response response = { 0 };
  uint8_t response_buf[500] = { 0 };
  ignore = az_http_response_init(&response, AZ_SPAN_FROM_BUFFER(response_buf));

#ifdef MOCK_ENABLED
  {
    will_return(__wrap_az_platform_clock_msec, 1 * 100000000);

    ignore = az_http_pipeline_process(&pipeline, &request, &response);
    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\Response1"), response._internal.http_response));

    ignore = az_http_pipeline_process(&pipeline, &request, &response);
    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\Response2"), response._internal.http_response));
  }
  {
    will_return(__wrap_az_platform_clock_msec, 2 * 100000000);

    ignore = az_http_pipeline_process(&pipeline, &request, &response);
    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\Response3"), response._internal.http_response));

    ignore = az_http_pipeline_process(&pipeline, &request, &response);
    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\Response4"), response._internal.http_response));
  }
#else
  (void)pipeline;
#endif // MOCK_ENABLED

  (void)ignore;
}

az_result send_request(_az_http_request* request, az_http_response* response);

az_result send_request(_az_http_request* request, az_http_response* response)
{
  static bool redo_auth = false;

  az_span const request_url
      = az_span_slice(request->_internal.url, 0, request->_internal.url_length);

  if (!az_span_is_content_equal(
          AZ_SPAN_FROM_STR("https://www.microsoft.com/test/request"),
          request_url)) // Auth request
  {
    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_STR("https://login.microsoftonline.com/TenantID/oauth2/v2.0/token"),
        request_url));

    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_STR("grant_type=client_credentials"
                         "&client_id=ClientID"
                         "&scope=Scopes"
                         "&client_secret=ClientSecret"),
        request->_internal.body));

    assert_int_equal(1, _az_http_request_headers_count(request));
    {
      az_pair header = { 0 };

      az_result const ignore = az_http_request_get_header(request, 0, &header);
      (void)ignore;

      assert_true(az_span_is_content_equal(AZ_SPAN_FROM_STR("Content-Type"), header.key));
      assert_true(az_span_is_content_equal(
          AZ_SPAN_FROM_STR("application/x-www-form-urlencoded"), header.value));
    }

    static int auth_attempt = 0;
    if (redo_auth)
    {
      auth_attempt = 0;
    }

    ++auth_attempt;

    // 3rd attempt should never happen because the token should not be expiring given that clock
    // returns 0.
    assert_in_range(auth_attempt, 1, 2);

    if (auth_attempt == 1)
    {
      response->_internal.http_response
          = AZ_SPAN_FROM_STR("HTTP/1.1 500 Internal Server Error\r\n\r\n");
    }
    else
    {
      if (!redo_auth)
      {
        response->_internal.http_response = AZ_SPAN_FROM_STR(
            "HTTP/1.1 200 OK\r\n\r\n{ 'access_token' : 'AccessToken', 'expires_in' : 3600 }");
      }
      else
      {
        response->_internal.http_response = AZ_SPAN_FROM_STR(
            "HTTP/1.1 200 OK\r\n\r\n{ 'access_token' : 'NewAccessToken', 'expires_in' : 3600 }");
      }
    }
  }
  else // The actual HTTP request
  {
    bool has_auth_header = false;
    int32_t const header_count = _az_http_request_headers_count(request);
    for (int32_t i = 0; i < header_count; ++i)
    {
      az_pair header = { 0 };

      az_result const ignore = az_http_request_get_header(request, i, &header);
      (void)ignore;

      if (az_span_is_content_equal(AZ_SPAN_FROM_STR("authorization"), header.key))
      {
        if (!redo_auth)
        {
          assert_true(
              az_span_is_content_equal(AZ_SPAN_FROM_STR("Bearer AccessToken"), header.value));
        }
        else
        {
          assert_true(
              az_span_is_content_equal(AZ_SPAN_FROM_STR("Bearer NewAccessToken"), header.value));
        }

        has_auth_header = true;
      }
    }

    assert_true(has_auth_header);

    static int attempt = 0;
    ++attempt

    if (attempt == 1)
    {
      response->_internal.http_response = AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\Response1");
      first_attempt = false;
    }
    else if (attempt == 2)
    {
      response->_internal.http_response = AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\Response2");
      redo_auth = true;
    }
    if (attempt == 3 && redo_auth)
    {
      response->_internal.http_response = AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\Response3");
    }
    else if (attempt == 4 && redo_auth)
    {
      response->_internal.http_response = AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\Response4");
      redo_auth = true;
    }
  }

  return AZ_OK;
}

#ifdef MOCK_ENABLED
az_result __wrap_az_http_client_send_request(_az_http_request* request, az_http_response* response);
int64_t __wrap_az_platform_clock_msec();

az_result __wrap_az_http_client_send_request(_az_http_request* request, az_http_response* response)
{
  return send_request(request, response);
}

int64_t __wrap_az_platform_clock_msec() { return (int64_t)mock(); }
#endif // MOCK_ENABLED

int test_az_credential_client_secret()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_credential_client_secret),
  };
  return cmocka_run_group_tests_name("az_core_credential_client_secret", tests, NULL, NULL);
}
