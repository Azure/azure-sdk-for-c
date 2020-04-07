// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_http_policy_private.h"
#include "az_test_definitions.h"
#include <az_credentials.h>
#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_transport.h>
#include <az_span.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <_az_cfg.h>

#ifdef MOCK_ENABLED
az_result test_policy_transport_retry_response(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response);

az_result test_policy_transport_retry_response_with_header(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response);

az_result test_policy_transport_retry_response_with_header_2(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response);
void test_az_http_pipeline_policy_credential();
void test_az_http_pipeline_policy_retry();
void test_az_http_pipeline_policy_retry_with_header();
void test_az_http_pipeline_policy_retry_with_header_2();
#endif // MOCK_ENABLED

static az_result test_policy_transport(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response);

void test_az_http_pipeline_policy_apiversion();
void test_az_http_pipeline_policy_uniquerequestid();
void test_az_http_pipeline_policy_telemetry();

void test_az_http_policy(void** state)
{
  (void)state;

/* Tests using wrap to mock. Only suported by gcc */
#ifdef MOCK_ENABLED
  test_az_http_pipeline_policy_credential();
  test_az_http_pipeline_policy_retry();
  test_az_http_pipeline_policy_retry_with_header();
  test_az_http_pipeline_policy_retry_with_header_2();
#endif // MOCK_ENABLED

  test_az_http_pipeline_policy_apiversion();
  test_az_http_pipeline_policy_uniquerequestid();
  test_az_http_pipeline_policy_telemetry();
}

az_result test_policy_transport(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response)
{
  (void)p_policies;
  (void)p_options;
  (void)p_request;
  (void)p_response;
  return AZ_OK;
}

void test_az_http_pipeline_policy_telemetry()
{
  uint8_t buf[100];
  uint8_t header_buf[(2 * sizeof(az_pair))];
  memset(buf, 0, sizeof(buf));
  memset(header_buf, 0, sizeof(header_buf));

  az_span url_span = AZ_SPAN_FROM_BUFFER(buf);
  url_span = az_span_append(url_span, AZ_SPAN_FROM_STR("url"));
  assert_int_equal(az_span_capacity(url_span) + 3, 100);
  az_span header_span = AZ_SPAN_FROM_BUFFER(header_buf);
  _az_http_request hrb;

  assert_return_code(
      az_http_request_init(
          &hrb, &az_context_app, az_http_method_get(), url_span, header_span, AZ_SPAN_NULL),
      AZ_OK);

  // Create policy options
  _az_http_policy_telemetry_options telemetry = _az_http_policy_telemetry_options_default();

  _az_http_policy policies[1] = {            
            {
              ._internal = {
                .process = test_policy_transport,
                .p_options = NULL,
              },
            },
        };

  assert_return_code(az_http_pipeline_policy_telemetry(policies, &telemetry, &hrb, NULL), AZ_OK);
}

void test_az_http_pipeline_policy_apiversion()
{
  uint8_t buf[100];
  uint8_t header_buf[(2 * sizeof(az_pair))];
  memset(buf, 0, sizeof(buf));
  memset(header_buf, 0, sizeof(header_buf));

  az_span url_span = AZ_SPAN_FROM_BUFFER(buf);
  url_span = az_span_append(url_span, AZ_SPAN_FROM_STR("url"));
  assert_int_equal(az_span_capacity(url_span) + 3, 100);
  az_span header_span = AZ_SPAN_FROM_BUFFER(header_buf);
  _az_http_request hrb;

  assert_return_code(
      az_http_request_init(
          &hrb, &az_context_app, az_http_method_get(), url_span, header_span, AZ_SPAN_NULL),
      AZ_OK);

  // Create policy options
  _az_http_policy_apiversion_options api_version = _az_http_policy_apiversion_options_default();
  api_version._internal.option_location = _az_http_policy_apiversion_option_location_queryparameter;
  api_version._internal.name = AZ_SPAN_FROM_STR("name");
  api_version._internal.version = AZ_SPAN_FROM_STR("version");

  _az_http_policy policies[1] = {            
            {
              ._internal = {
                .process = test_policy_transport,
                .p_options = NULL,
              },
            },
        };

  // make sure token is not expired
  assert_return_code(az_http_pipeline_policy_apiversion(policies, &api_version, &hrb, NULL), AZ_OK);

  api_version._internal.option_location = _az_http_policy_apiversion_option_location_header;
  assert_return_code(az_http_pipeline_policy_apiversion(policies, &api_version, &hrb, NULL), AZ_OK);
}

void test_az_http_pipeline_policy_uniquerequestid()
{
  uint8_t buf[100];
  uint8_t header_buf[(2 * sizeof(az_pair))];
  memset(buf, 0, sizeof(buf));
  memset(header_buf, 0, sizeof(header_buf));

  az_span url_span = AZ_SPAN_FROM_BUFFER(buf);
  url_span = az_span_append(url_span, AZ_SPAN_FROM_STR("url"));
  assert_int_equal(az_span_capacity(url_span) + 3, 100);
  az_span header_span = AZ_SPAN_FROM_BUFFER(header_buf);
  _az_http_request hrb;

  assert_return_code(
      az_http_request_init(
          &hrb, &az_context_app, az_http_method_get(), url_span, header_span, AZ_SPAN_NULL),
      AZ_OK);

  _az_http_policy policies[1] = {            
            {
              ._internal = {
                .process = test_policy_transport,
                .p_options = NULL,
              },
            },
        };

  // make sure token is not expired
  assert_return_code(az_http_pipeline_policy_uniquerequestid(policies, NULL, &hrb, NULL), AZ_OK);
}

#ifdef MOCK_ENABLED

const az_span retry_response = AZ_SPAN_LITERAL_FROM_STR("HTTP/1.1 408 Request Timeout\r\n"
                                                        "Content-Type: text/html; charset=UTF-8\r\n"
                                                        "\r\n"
                                                        "{\r\n"
                                                        "  \"body\":0,\r"
                                                        "}\n");

const az_span retry_response_with_header
    = AZ_SPAN_LITERAL_FROM_STR("HTTP/1.1 408 Request Timeout\r\n"
                               "Content-Type: text/html; charset=UTF-8\r\n"
                               "retry-after-ms: 1600\r\n"
                               "\r\n"
                               "{\r\n"
                               "  \"body\":0,\r"
                               "}\n");

const az_span retry_response_with_header_2
    = AZ_SPAN_LITERAL_FROM_STR("HTTP/1.1 408 Request Timeout\r\n"
                               "Content-Type: text/html; charset=UTF-8\r\n"
                               "Retry-After: 1600\r\n"
                               "\r\n"
                               "{\r\n"
                               "  \"body\":0,\r"
                               "}\n");

az_result test_policy_transport_retry_response(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response)
{
  (void)p_policies;
  (void)p_options;
  (void)p_request;
  assert_return_code(az_http_response_init(p_response, retry_response), AZ_OK);
  return AZ_OK;
}

az_result test_policy_transport_retry_response_with_header(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response)
{
  (void)p_policies;
  (void)p_options;
  (void)p_request;
  assert_return_code(az_http_response_init(p_response, retry_response_with_header), AZ_OK);
  return AZ_OK;
}

az_result test_policy_transport_retry_response_with_header_2(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response)
{
  (void)p_policies;
  (void)p_options;
  (void)p_request;
  assert_return_code(az_http_response_init(p_response, retry_response_with_header_2), AZ_OK);
  return AZ_OK;
}

void test_az_http_pipeline_policy_credential()
{
  uint8_t buf[100];
  uint8_t header_buf[(2 * sizeof(az_pair))];
  memset(buf, 0, sizeof(buf));
  memset(header_buf, 0, sizeof(header_buf));

  az_span url_span = AZ_SPAN_FROM_BUFFER(buf);
  url_span = az_span_append(url_span, AZ_SPAN_FROM_STR("url"));
  assert_int_equal(az_span_capacity(url_span) + 3, 100);
  az_span header_span = AZ_SPAN_FROM_BUFFER(header_buf);
  _az_http_request hrb;

  assert_return_code(
      az_http_request_init(
          &hrb, &az_context_app, az_http_method_get(), url_span, header_span, AZ_SPAN_NULL),
      AZ_OK);

  // Create a credential sample
  az_credential_client_secret credential = { 0 };
  assert_return_code(
      az_credential_client_secret_init(
          &credential,
          AZ_SPAN_FROM_STR("id"),
          AZ_SPAN_FROM_STR("tenant"),
          AZ_SPAN_FROM_STR("secret")),
      AZ_OK);

  _az_http_policy policies[1] = {            
            {
              ._internal = {
                .process = test_policy_transport,
                .p_options = NULL,
              },
            },
        };

  // make sure token is not expired
  will_return(__wrap_az_platform_clock_msec, 0);
  assert_return_code(az_http_pipeline_policy_credential(policies, &credential, &hrb, NULL), AZ_OK);
}

void test_az_http_pipeline_policy_retry()
{
  uint8_t buf[100];
  uint8_t header_buf[(2 * sizeof(az_pair))];
  memset(buf, 0, sizeof(buf));
  memset(header_buf, 0, sizeof(header_buf));

  az_span url_span = AZ_SPAN_FROM_BUFFER(buf);
  url_span = az_span_append(url_span, AZ_SPAN_FROM_STR("url"));
  assert_int_equal(az_span_capacity(url_span) + 3, 100);
  az_span header_span = AZ_SPAN_FROM_BUFFER(header_buf);
  _az_http_request hrb;

  assert_return_code(
      az_http_request_init(
          &hrb, &az_context_app, az_http_method_get(), url_span, header_span, AZ_SPAN_NULL),
      AZ_OK);

  // Create policy options
  az_http_policy_retry_options retry_options = _az_http_policy_retry_options_default();

  _az_http_policy policies[1] = {            
            {
              ._internal = {
                .process = test_policy_transport_retry_response,
                .p_options = NULL,
              },
            },
        };

  // set clock sec required when retrying (will retry 4 times)
  will_return(__wrap_az_platform_clock_msec, 0);
  will_return(__wrap_az_platform_clock_msec, 0);
  will_return(__wrap_az_platform_clock_msec, 0);
  will_return(__wrap_az_platform_clock_msec, 0);
  az_http_response response;
  assert_return_code(
      az_http_pipeline_policy_retry(policies, &retry_options, &hrb, &response), AZ_OK);
}

void test_az_http_pipeline_policy_retry_with_header()
{
  uint8_t buf[100];
  uint8_t header_buf[(2 * sizeof(az_pair))];
  memset(buf, 0, sizeof(buf));
  memset(header_buf, 0, sizeof(header_buf));

  az_span url_span = AZ_SPAN_FROM_BUFFER(buf);
  url_span = az_span_append(url_span, AZ_SPAN_FROM_STR("url"));
  assert_int_equal(az_span_capacity(url_span) + 3, 100);
  az_span header_span = AZ_SPAN_FROM_BUFFER(header_buf);
  _az_http_request hrb;

  assert_return_code(
      az_http_request_init(
          &hrb, &az_context_app, az_http_method_get(), url_span, header_span, AZ_SPAN_NULL),
      AZ_OK);

  // Create policy options
  az_http_policy_retry_options retry_options = _az_http_policy_retry_options_default();
  // make just one retry
  retry_options.max_retries = 1;

  _az_http_policy policies[1] = {            
            {
              ._internal = {
                .process = test_policy_transport_retry_response_with_header,
                .p_options = NULL,
              },
            },
        };

  // set clock sec required when retrying (will retry 4 times)
  will_return(__wrap_az_platform_clock_msec, 0);

  az_http_response response;
  assert_return_code(
      az_http_pipeline_policy_retry(policies, &retry_options, &hrb, &response), AZ_OK);
}

void test_az_http_pipeline_policy_retry_with_header_2()
{
  uint8_t buf[100];
  uint8_t header_buf[(2 * sizeof(az_pair))];
  memset(buf, 0, sizeof(buf));
  memset(header_buf, 0, sizeof(header_buf));

  az_span url_span = AZ_SPAN_FROM_BUFFER(buf);
  url_span = az_span_append(url_span, AZ_SPAN_FROM_STR("url"));
  assert_int_equal(az_span_capacity(url_span) + 3, 100);
  az_span header_span = AZ_SPAN_FROM_BUFFER(header_buf);
  _az_http_request hrb;

  assert_return_code(
      az_http_request_init(
          &hrb, &az_context_app, az_http_method_get(), url_span, header_span, AZ_SPAN_NULL),
      AZ_OK);

  // Create policy options
  az_http_policy_retry_options retry_options = _az_http_policy_retry_options_default();
  // make just one retry
  retry_options.max_retries = 1;

  _az_http_policy policies[1] = {            
            {
              ._internal = {
                .process = test_policy_transport_retry_response_with_header_2,
                .p_options = NULL,
              },
            },
        };

  // set clock sec required when retrying (will retry 4 times)
  will_return(__wrap_az_platform_clock_msec, 0);

  az_http_response response;
  assert_return_code(
      az_http_pipeline_policy_retry(policies, &retry_options, &hrb, &response), AZ_OK);
}

#endif // MOCK_ENABLED
