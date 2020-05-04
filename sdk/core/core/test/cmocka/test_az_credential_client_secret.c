// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <az_credentials.h>
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

  az_span const expected_responses[] = {
    AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\nResponse1"),
    AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\nResponse2"),
    AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\nResponse3"),
    AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\nResponse4"),
  };

  // Cmocka works in a way that you have to pre-load it with a value for every time it will be
  // invoked (it does not return previously set value).
  int const clock_nrequests[] = {
    2, // wait to retry, set token expiration
    1, // check if token has expired
    3, // check if token has expired, wait to retry, set token expiration
    1, // check if token has expired
  };

  // Some value that is big enough so that when you add 3600000 milliseconds to it (1 hour),
  // and while in debuger, the vallue you see is seen as "103600000", which is easy to debug.
  int const clock_value[] = {
    100000000, // first - initial request. Token will be obtained
    100000000, // the token is not expected to expire, cached value will be used.
    200000000, // token should be considered expired, when clock is this, so it should refresh.
    200000000, // use cached refreshed token.
  };

  az_span const request_url = AZ_SPAN_FROM_STR("https://www.microsoft.com/test/request");
  for (int i = 0; i < 4; ++i)
  {
    az_result ignore = { 0 };

    uint8_t header_buf[500] = { 0 };
    uint8_t body_buf[500] = { 0 };
    _az_http_request request = { 0 };
    ignore = az_http_request_init(
        &request,
        &az_context_app,
        az_http_method_get(),
        request_url,
        az_span_size(request_url),
        AZ_SPAN_FROM_BUFFER(header_buf),
        AZ_SPAN_FROM_BUFFER(body_buf));

    az_http_response response = { 0 };
    uint8_t response_buf[500] = { 0 };
    ignore = az_http_response_init(&response, AZ_SPAN_FROM_BUFFER(response_buf));

#ifdef _az_MOCK_ENABLED
    will_return_count(__wrap_az_platform_clock_msec, clock_value[i], clock_nrequests[i]);
    ignore = az_http_pipeline_process(&pipeline, &request, &response);
    assert_true(az_span_is_content_equal(expected_responses[i], response._internal.http_response));
#else // _az_MOCK_ENABLED
    (void)pipeline;
    (void)expected_responses;
    (void)clock_nrequests;
    (void)clock_value;
#endif // _az_MOCK_ENABLED
    (void)ignore;
  }
}

az_result send_request(_az_http_request* request, az_http_response* response);

az_result send_request(_az_http_request* request, az_http_response* response)
{
  // This function handles requests to both auth service and to the supposed service itself.
  // (we only can inject at compile time).

  // This is used to indicate whether we should be returning and expecting the "AuthToken" or
  // "NewAuthToken" (supposedly the one that you get requesting a token an hour later).
  static bool redo_auth = false;

  az_span request_url = { 0 };
  assert_true(az_http_request_get_url(request, &request_url));
  az_span body = { 0 };
  assert_true(az_http_request_get_body(request, &body));

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
        body));

    assert_int_equal(1, az_http_request_headers_count(request));
    {
      az_pair header = { 0 };

      az_result const ignore = az_http_request_get_header(request, 0, &header);
      (void)ignore;

      assert_true(az_span_is_content_equal(AZ_SPAN_FROM_STR("Content-Type"), header.key));
      assert_true(az_span_is_content_equal(
          AZ_SPAN_FROM_STR("application/x-www-form-urlencoded"), header.value));
    }

    static int auth_attempt = 0;
    if (redo_auth && auth_attempt == 2)
    {
      auth_attempt = 0;
    }

    ++auth_attempt;

    // 3rd attempt to request a token should never happen because we expect the token to be cached.
    // (Unless we are simulating an expired token, which is controlled by redo_auth static variable
    // in this function).
    assert_in_range(auth_attempt, 1, 2);

    if (auth_attempt == 1)
    {
      // Simulate a retriable HTTP error during a first attempt to get token.
      response->_internal.http_response
          = AZ_SPAN_FROM_STR("HTTP/1.1 500 Internal Server Error\r\n\r\n");
    }
    else
    {
      if (!redo_auth)
      {
        // "Initial" token.
        response->_internal.http_response
            = AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\n"
                               "{ \"access_token\" : \"AccessToken\", \"expires_in\" : 3600 }");
      }
      else
      {
        // "New" token.
        response->_internal.http_response
            = AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\n"
                               "{ \"access_token\" : \"NewAccessToken\", \"expires_in\" : 3600 }");
      }
    }
  }
  else // The actual HTTP request
  {
    bool has_auth_header = false;
    int32_t const header_count = az_http_request_headers_count(request);
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
        else // Verify that we've got the refreshed token
        {
          assert_true(
              az_span_is_content_equal(AZ_SPAN_FROM_STR("Bearer NewAccessToken"), header.value));
        }

        has_auth_header = true;
      }
    }

    assert_true(has_auth_header);

    static int attempt = 0;
    ++attempt;

    // Return different values so that it is more likely that the test wenth through all
    // verifications (both old and new token).
    if (attempt == 1)
    {
      response->_internal.http_response = AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\nResponse1");
    }
    else if (attempt == 2)
    {
      response->_internal.http_response = AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\nResponse2");
      redo_auth = true;
    }
    if (attempt == 3 && redo_auth)
    {
      response->_internal.http_response = AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\nResponse3");
    }
    else if (attempt == 4 && redo_auth)
    {
      response->_internal.http_response = AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n\r\nResponse4");
      redo_auth = false;
    }
  }

  return AZ_OK;
}

#ifdef _az_MOCK_ENABLED
az_result __wrap_az_http_client_send_request(_az_http_request* request, az_http_response* response);
int64_t __wrap_az_platform_clock_msec();

az_result __wrap_az_http_client_send_request(_az_http_request* request, az_http_response* response)
{
  return send_request(request, response);
}

int64_t __wrap_az_platform_clock_msec() { return (int64_t)mock(); }
#endif // _az_MOCK_ENABLED

int test_az_credential_client_secret()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_credential_client_secret),
  };
  return cmocka_run_group_tests_name("az_core_credential_client_secret", tests, NULL, NULL);
}
