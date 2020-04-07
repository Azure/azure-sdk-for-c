// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_http_policy_private.h"
#include "az_test_definitions.h"
#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_transport.h>
#include <az_span.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <_az_cfg.h>

// define 2 test policies
az_result test_policy_1(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response);

az_result test_policy_2(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response);

void test_az_http_pipeline_process();

void test_az_pipeline(void** state)
{
  (void)state;

  test_az_http_pipeline_process();
}

void test_az_http_pipeline_process()
{
  uint8_t buf[100];
  uint8_t header_buf[(2 * sizeof(az_pair))];
  memset(buf, 0, sizeof(buf));
  memset(header_buf, 0, sizeof(header_buf));

  az_span url_span = AZ_SPAN_FROM_BUFFER(buf);
  url_span = az_span_append(url_span, AZ_SPAN_FROM_STR("url"));
  assert_int_equal(az_span_length(url_span), 3);
  az_span header_span = AZ_SPAN_FROM_BUFFER(header_buf);
  _az_http_request hrb;

  assert_return_code(
      az_http_request_init(
          &hrb, &az_context_app, az_http_method_get(), url_span, header_span, AZ_SPAN_NULL),
      AZ_OK);

  _az_http_pipeline pipeline = (_az_http_pipeline){
        ._internal = {
          .p_policies = {
            {
              ._internal = {
                .process = test_policy_1,
                .p_options= NULL,
              },
            },
            {
              ._internal = {
                .process = test_policy_2,
                .p_options = NULL,
              },
            },
        },
      },
  };

  uint8_t buffer[10];
  az_span response_buffer = AZ_SPAN_FROM_BUFFER(buffer);
  az_http_response response;
  assert_return_code(az_http_response_init(&response, response_buffer), AZ_OK);

  assert_return_code(az_http_pipeline_process(&pipeline, &hrb, &response), AZ_OK);
}

az_result test_policy_1(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response)
{
  (void)p_policies;
  (void)p_options;
  (void)p_request;
  (void)p_response;
  return az_http_pipeline_nextpolicy(p_policies, p_request, p_response);
}

az_result test_policy_2(
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
