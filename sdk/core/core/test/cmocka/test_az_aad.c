// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

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
// int64_t __wrap_az_platform_clock_msec();
int64_t __wrap_az_platform_clock_msec() { return (int64_t)mock(); }
az_http_response mocked_http_response;
az_result __wrap_az_http_pipeline_policy_transport(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response)
{
  (void)p_policies;
  (void)p_options;
  (void)p_request;
}

void test_az_token_expired();
void test_az_aad_request_token();

void test_az_aad(void** state)
{
  (void)state;

  test_az_token_expired();
  test_az_aad_request_token();
}

void test_az_token_expired()
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

void test_az_aad_request_token()
{
  // Calling az_aad_request_token(_az_http_request* ref_request, _az_token* out_token);
}