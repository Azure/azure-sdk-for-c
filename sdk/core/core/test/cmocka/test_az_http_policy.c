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

void test_az_http_policy(void** state)
{
  (void)state;

/* Tests using wrap to mock. Only suported by gcc */
#ifdef MOCK_ENABLED
  test_az_http_pipeline_policy_credential();
#endif // MOCK_ENABLED
}

void test_az_http_pipeline_policy_credential()
{
  uint8_t buf[100];
  uint8_t header_buf[(2 * sizeof(az_pair))];
  memset(buf, 0, sizeof(buf));
  memset(header_buf, 0, sizeof(header_buf));

  az_span url_span = AZ_SPAN_FROM_BUFFER(buf);
  assert_return_code(az_span_append(url_span, AZ_SPAN_FROM_STR("url"), &url_span), AZ_OK);
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
