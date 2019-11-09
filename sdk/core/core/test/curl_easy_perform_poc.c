// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_auth.h>
#include <az_http_client.h>
#include <az_http_request_builder.h>
#include <az_http_response_parser.h>
#include <az_pair.h>
#include <az_span.h>
#include <az_span_builder.h>
#include <az_span_malloc.h>

#include <az_http_pipeline.h>

#include <stdio.h>
#include <stdlib.h>

#include "./az_test.h"
int exit_code = 0;

#include <_az_cfg.h>

static az_span KEY_VAULT_URL
    = AZ_CONST_STR("https://antk-keyvault.vault.azure.net/secrets/Password");

static az_span API_VERSION_QUERY_NAME = AZ_CONST_STR("api-version");
static az_span API_VERSION_QUERY_VALUE = AZ_CONST_STR("7.0");

int main() {
  // create a buffer for request
  uint8_t buf[1024 * 4];
  az_mut_span const http_buf = AZ_SPAN_FROM_ARRAY(buf);
  az_http_request_builder hrb;

  // response buffer
  uint8_t buf_response[1024 * 4];
  az_mut_span const http_buf_response = AZ_SPAN_FROM_ARRAY(buf_response);

  az_auth_credentials creds = { { 0 } };
  az_result const creds_retcode = az_auth_init_client_credentials(
      &creds,
      AZ_STR("72f988bf-86f1-41af-91ab-2d7cd011db47"),
      AZ_STR("4317a660-6bfb-4585-9ce9-8f222314879c"),
      AZ_STR("O2CT[Y:dkTqblml5V/T]ZEi9x1W1zoBW"));

  if (!az_succeeded(creds_retcode)) {
    printf("Error initializing credentials\n");
    return creds_retcode;
  }

  az_span auth_token = { 0 };
  az_result const token_retcode
      = az_auth_get_token(creds, AZ_STR("https://vault.azure.net"), http_buf_response, &auth_token);

  if (az_failed(token_retcode)) {
    printf("Error while getting auth token\n");
    return token_retcode;
  }

  // create request for keyVault
  az_result build_result
      = az_http_request_builder_init(&hrb, http_buf, 100, AZ_HTTP_METHOD_VERB_GET, KEY_VAULT_URL);
  if (az_failed(build_result)) {
    return build_result;
  }

  // add query param
  az_result add_query_result = az_http_request_builder_set_query_parameter(
      &hrb, API_VERSION_QUERY_NAME, API_VERSION_QUERY_VALUE);
  if (az_failed(add_query_result)) {
    return add_query_result;
  }

  /****** -------------  Create buffer for header auth ---------******/
  // can't print auth_token right now since it is not 0-terminated
  size_t const buffer_for_header_size = sizeof("Bearer ") + auth_token.size;
  az_mut_span temp_buf;
  TEST_EXPECT_SUCCESS(az_span_malloc(buffer_for_header_size, &temp_buf));

  /****** -------------  use Span builder to concatenate ---------******/
  az_span_builder builder = az_span_builder_create(temp_buf);
  TEST_EXPECT_SUCCESS(az_span_builder_append(&builder, AZ_STR("Bearer ")));
  TEST_EXPECT_SUCCESS(az_span_builder_append(&builder, auth_token));
  TEST_EXPECT_SUCCESS(az_span_builder_append(
      &builder, AZ_STR_ZERO)); // add a 0 so it can be printed and used by Curl

  // add auth Header with parsed auth_token
  az_result const add_header_result = az_http_request_builder_append_header(
      &hrb, AZ_STR("authorization"), az_mut_span_to_span(temp_buf));
  if (az_failed(add_header_result)) {
    return add_header_result;
  }

  // *************************send GET
  az_result const get_response = az_http_client_send_request(&hrb, &http_buf_response);
  if (az_failed(get_response)) {
    printf("Error after request key from KeyVault\n");
    return get_response;
  }

  if (az_succeeded(get_response)) {
    printf("Response is: \n%s", http_buf_response.begin);
  } else {
    printf("Error during running test\n");
    printf("Response is: \n%s", http_buf_response.begin);
    return get_response;
  }

  // free the temporal buffer holding auth token
  TEST_EXPECT_SUCCESS(az_mut_span_memset(temp_buf, 0));
  az_span_free(&temp_buf);

  az_result result;
  result = az_http_pipeline_process(&hrb, &http_buf_response);

  return 0;
}
